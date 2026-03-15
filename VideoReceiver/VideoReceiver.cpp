#include "VideoReceiver.hpp"

#include <cstring>

ReceiverWorker::ReceiverWorker(QObject *parent) : QObject(parent) {
}

void ReceiverWorker::init() {
    m_udpReceiver = new QUdpSocket(this);

    // Привязка сокета к порту приема для всех доступных сетевых интерфейсов (IPv4).
    m_udpReceiver->bind(QHostAddress(QHostAddress::AnyIPv4), 5555);

    // Расширение системного буфера приема ОС для минимизации потерь датаграмм при высоких сетевых нагрузках.
    m_udpReceiver->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024 * 8);

    connect(m_udpReceiver, &QUdpSocket::readyRead, this, &ReceiverWorker::readPendingDatagrams);
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

    // Механизм толерантности к потерям пакетов (Lossy Streaming).
    // При поступлении фрагмента от более позднего кадра, процесс сборки текущего прекращается,
    // и частично собранный кадр принудительно отправляется на рендеринг во избежание накопления задержки.
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

    // Очистка устаревших буферов кадров для предотвращения утечек оперативной памяти.
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

        // Преаллокация буфера и заполнение нулевыми байтами (черный цвет)
        // для визуального замещения недошедших по сети фрагментов.
        buffer.data.fill(0, hdr.imgWidth * hdr.imgHeight * 4);
    }

    // Копирование полезной нагрузки фрагмента в аллоцированную память по вычисленному смещению.
    qsizetype offset = hdr.fragmentIdx * CHUNK_SIZE;
    if (offset + packet.payload.size() <= buffer.data.size()) {
        std::memcpy(buffer.data.data() + offset, packet.payload.constData(), packet.payload.size());
        buffer.chunksReceived++;
    }
}

VideoReceiver::VideoReceiver(QObject *parent) : QObject(parent) {
    // Регистрация типа QImage для корректной маршрутизации данных через систему сигналов и слотов между потоками.
    qRegisterMetaType<QImage>("QImage");

    m_worker = new ReceiverWorker();
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &ReceiverWorker::init);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ReceiverWorker::frameAssembled, this, &VideoReceiver::onFrameAssembled);

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
