#include "VideoSender.hpp"

#include <algorithm>

SenderWorker::SenderWorker(QObject *parent) : QObject(parent) {}

void SenderWorker::init()
{
    m_udpSender = new QUdpSocket(this);

    // Привязка к любому доступному локальному порту для возможности настройки опций сокета
    m_udpSender->bind(QHostAddress(QHostAddress::LocalHost), 0);

    // Расширение буфера отправки ОС до 8 МБ
    m_udpSender->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024 * 8);
}

void SenderWorker::processImage(QImage img)
{
    const uchar *bits = img.constBits();
    qsizetype sizeBytes = img.sizeInBytes();

    quint16 totalFragments = (sizeBytes + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_frameCounter++;

    for (quint16 i = 0; i < totalFragments; ++i)
    {
        PacketData packet;
        packet.header.type = MsgType::VideoFrame;
        packet.header.frameId = m_frameCounter;
        packet.header.fragmentIdx = i;
        packet.header.totalFragments = totalFragments;
        packet.header.imgWidth = img.width();
        packet.header.imgHeight = img.height();

        qsizetype offset = i * CHUNK_SIZE;
        qsizetype currentChunkSize = std::min<qsizetype>(CHUNK_SIZE, sizeBytes - offset);

        packet.payload = QByteArray(reinterpret_cast<const char *>(bits + offset), currentChunkSize);

        // Отправка датаграммы на локальный адрес клиента
        m_udpSender->writeDatagram(packet.toQBA(), QHostAddress(QHostAddress::LocalHost), 5555);
    }

    emit readyForNextFrame();
}

VideoSender::VideoSender(QObject *parent) : QObject(parent)
{
    // Регистрация пользовательского типа для системы метаобъектов Qt (Signals/Slots)
    qRegisterMetaType<QImage>("QImage");

    m_worker = new SenderWorker();
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &SenderWorker::init);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &VideoSender::dispatchImage, m_worker, &SenderWorker::processImage);
    connect(m_worker, &SenderWorker::readyForNextFrame, this, [this]() { m_isWorkerBusy = false; });

    m_thread.start();
}

VideoSender::~VideoSender()
{
    m_thread.quit();
    m_thread.wait();
}

QVideoSink *VideoSender::sourceSink() const
{
    return m_sourceSink;
}

void VideoSender::setSourceSink(QVideoSink *sink)
{
    if (m_sourceSink == sink)
    {
        return;
    }

    if (m_sourceSink)
    {
        disconnect(m_sourceSink, &QVideoSink::videoFrameChanged, this, &VideoSender::processLocalFrame);
    }

    m_sourceSink = sink;

    if (m_sourceSink)
    {
        connect(m_sourceSink, &QVideoSink::videoFrameChanged, this, &VideoSender::processLocalFrame);
    }

    emit sourceSinkChanged();
}

bool VideoSender::active() const
{
    return m_active;
}

void VideoSender::setActive(bool active)
{
    if (m_active == active)
    {
        return;
    }

    m_active = active;
    emit activeChanged();
}

void VideoSender::processLocalFrame(const QVideoFrame &frame)
{
    // Проверка активности трансляции и валидности кадра, а также защита от переполнения очереди
    if (!m_active || !frame.isValid() || m_isWorkerBusy)
    {
        return;
    }

    m_isWorkerBusy = true;

    QVideoFrame f = frame;

    // Блокировка кадра для чтения и маппинг данных из графического ускорителя в ОЗУ
    if (!f.map(QVideoFrame::ReadOnly))
    {
        m_isWorkerBusy = false;
        return;
    }

    QImage img = f.toImage();
    f.unmap();

    if (!img.isNull())
    {
        // Масштабирование кадра для оптимизации сетевого трафика
        img = img.scaled(320, 240, Qt::KeepAspectRatio).convertToFormat(QImage::Format_RGB32);
        emit dispatchImage(img);
    }
    else
    {
        m_isWorkerBusy = false;
    }
}
