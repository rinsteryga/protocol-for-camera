#include "VideoReceiver.hpp"

#include <cstring>
#include <QNetworkDatagram>
#include <QDebug>

// --- ReceiverWorker Implementation ---

ReceiverWorker::ReceiverWorker(QObject *parent) : QObject(parent) {
}

void ReceiverWorker::init() {
    m_udpReceiver = new QUdpSocket(this);

           // Привязка сокета к порту приема для всех доступных сетевых интерфейсов (IPv4).
    m_udpReceiver->bind(QHostAddress::AnyIPv4, 5556);

           // Расширение системного буфера приема ОС для минимизации потерь датаграмм.
    m_udpReceiver->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024 * 8);

    connect(m_udpReceiver, &QUdpSocket::readyRead, this, &ReceiverWorker::readPendingDatagrams);

    qInfo() << "ReceiverWorker: Socket initialized on port 5555";
}

void ReceiverWorker::sendStartRequest(const QString &ip) {
    if (ip.isEmpty()) {
        qWarning() << "ReceiverWorker: Target IP is empty. Request aborted.";
        return;
    }

    PacketData request;
    request.header.type = MsgType::Request; // Тип запроса из PacketHeader.hpp

    // Отправляем пустую структуру с заголовком Request на Orange Pi
    m_udpReceiver->writeDatagram(request.toQBA(), QHostAddress(ip), 5555);
    qInfo() << "ReceiverWorker: Handshake sent to" << ip;
}

void ReceiverWorker::readPendingDatagrams() {
    while (m_udpReceiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpReceiver->pendingDatagramSize());
        m_udpReceiver->readDatagram(datagram.data(), datagram.size());

        auto optPacket = PacketData::fromQBA(datagram);
        if (!optPacket) {
            continue;
        }

        handleIncomingPacket(optPacket.value());
    }
}

void ReceiverWorker::handleIncomingPacket(const PacketData& packet) {
    const auto& hdr = packet.header;
           // Игнорируем пакеты, если это не видеокадр (например, эхо наших же запросов)
    if (hdr.type != MsgType::VideoFrame) return;

           // Механизм толерантности к потерям пакетов (Lossy Streaming).
           // При поступлении фрагмента от более позднего кадра, завершаем сборку текущего.
    if (hdr.frameId > m_currentRenderedFrame) {
        auto it = m_frameBuffers.find(m_currentRenderedFrame);
        if (it != m_frameBuffers.end()) {
            auto& oldBuf = it->second;
            if (oldBuf.width > 0 && oldBuf.height > 0) {
                QImage img(reinterpret_cast<const uchar*>(oldBuf.data.constData()),
                           oldBuf.width, oldBuf.height, QImage::Format_RGB32);
                emit frameAssembled(img.copy());
            }
            m_frameBuffers.erase(it);
        }
        m_currentRenderedFrame = hdr.frameId;
    }

           // Очистка устаревших буферов кадров для предотвращения утечек памяти.
    if (m_frameBuffers.size() > 3) {
        quint32 threshold = (hdr.frameId > 3) ? (hdr.frameId - 3) : 0;
        for (auto it = m_frameBuffers.begin(); it != m_frameBuffers.end(); ) {
            if (it->first < threshold) {
                it = m_frameBuffers.erase(it);
            } else {
                ++it;
            }
        }
    }

    auto& buffer = m_frameBuffers[hdr.frameId];

           // Инициализация структуры буфера при поступлении первого фрагмента нового кадра.
    if (buffer.data.isEmpty()) {
        buffer.totalChunks = hdr.totalFragments;
        buffer.width = hdr.imgWidth;
        buffer.height = hdr.imgHeight;

               // Преаллокация буфера и заполнение нулевыми байтами (черный цвет).
        buffer.data.fill(0, hdr.imgWidth * hdr.imgHeight * 4);
    }

           // Копирование полезной нагрузки фрагмента в аллоцированную память по смещению.
    qsizetype offset = hdr.fragmentIdx * CHUNK_SIZE;
    if (offset + packet.payload.size() <= buffer.data.size()) {
        std::memcpy(buffer.data.data() + offset, packet.payload.constData(), packet.payload.size());
        buffer.chunksReceived++;
    }

           // Если кадр полностью собран - немедленно отправляем его на отрисовку
    if (buffer.chunksReceived == buffer.totalChunks) {
        QImage img(reinterpret_cast<const uchar*>(buffer.data.constData()),
                   buffer.width, buffer.height, QImage::Format_RGB32);
        emit frameAssembled(img.copy());
        m_frameBuffers.erase(hdr.frameId);
    }
}

// --- VideoReceiver Implementation ---

VideoReceiver::VideoReceiver(QObject *parent) : QObject(parent) {
    // Регистрация типа QImage для работы в сигналах/слотах между потоками.
    qRegisterMetaType<QImage>("QImage");

    m_worker = new ReceiverWorker();
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &ReceiverWorker::init);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    // Прием собранного кадра из воркера
    connect(m_worker, &ReceiverWorker::frameAssembled, this, &VideoReceiver::onFrameAssembled);

    // Передача IP из GUI в воркер для выполнения сетевого запроса
    connect(this, &VideoReceiver::requestStart, m_worker, &ReceiverWorker::sendStartRequest);

    m_thread.start();
}

VideoReceiver::~VideoReceiver() {
    m_thread.quit();
    m_thread.wait();
}

QImage VideoReceiver::currentFrame() const {
    return m_currentFrame;
}

void VideoReceiver::onFrameAssembled(const QImage &img) {
    m_currentFrame = img;
    emit currentFrameChanged();
}

void VideoReceiver::startStreaming() {
    // Этот метод вызывается из QML
    emit requestStart(m_serverIp);
}
