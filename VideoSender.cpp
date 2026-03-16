#include "VideoSender.hpp"

#include <algorithm>
#include <QDebug>
#include <QNetworkDatagram>

SenderWorker::SenderWorker(QObject *parent)
    : QObject(parent)
{
}

void SenderWorker::init() {
    m_udpSender = new QUdpSocket(this);

    m_udpSender->bind(QHostAddress::AnyIPv4, 5555);

    m_udpSender->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024 * 8);

    connect(m_udpSender, &QUdpSocket::readyRead, this, &SenderWorker::readPendingDatagrams);

    qInfo() << "SenderWorker: Инициализирован. Ожидание запроса на порт 5555...";
}

void SenderWorker::readPendingDatagrams() {
    while (m_udpSender->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSender->receiveDatagram();

        auto optPacket = PacketData::fromQBA(datagram.data());

        if (optPacket && optPacket->header.type == MsgType::Request) {
            m_targetAddress = datagram.senderAddress();
            m_targetPort = datagram.senderPort();
            m_isStreaming = true;

            qInfo() << "SenderWorker: Получен запрос START от" << m_targetAddress.toString() << ":" << m_targetPort;
        }
    }
}

void SenderWorker::processImage(QImage img) {
    if (!m_isStreaming) {
        emit readyForNextFrame();
        return;
    }

    const uchar* bits = img.constBits();
    qsizetype sizeBytes = img.sizeInBytes();

    quint16 totalFragments = (sizeBytes + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_frameCounter++;

    for (quint16 i = 0; i < totalFragments; ++i) {
        PacketData packet;
        packet.header.type = MsgType::VideoFrame;
        packet.header.frameId = m_frameCounter;
        packet.header.fragmentIdx = i;
        packet.header.totalFragments = totalFragments;
        packet.header.imgWidth = img.width();
        packet.header.imgHeight = img.height();

        qsizetype offset = i * CHUNK_SIZE;
        qsizetype currentChunkSize = std::min<qsizetype>(CHUNK_SIZE, sizeBytes - offset);

        packet.payload = QByteArray(reinterpret_cast<const char*>(bits + offset), currentChunkSize);

        m_udpSender->writeDatagram(packet.toQBA(), m_targetAddress, m_targetPort);
    }

    emit readyForNextFrame();
}


VideoSender::VideoSender(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QImage>("QImage");

    m_worker = new SenderWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::started, m_worker, &SenderWorker::init);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &VideoSender::dispatchImage, m_worker, &SenderWorker::processImage);
    connect(m_worker, &SenderWorker::readyForNextFrame, this, [this](){
        m_isWorkerBusy = false;
    });

    m_workerThread.start();

    auto cameras = QMediaDevices::videoInputs();
    if (!cameras.isEmpty()) {
        qInfo() << "VideoSender: Найдена камера:" << cameras.first().description();

        m_camera = new QCamera(cameras.first(), this);
        m_captureSession = new QMediaCaptureSession(this);

        m_sourceSink = new QVideoSink(this);

        m_captureSession->setCamera(m_camera);
        m_captureSession->setVideoSink(m_sourceSink);

        connect(m_sourceSink, &QVideoSink::videoFrameChanged, this, &VideoSender::processLocalFrame);

        m_active = true;
        m_camera->start();
    } else {
        qWarning() << "VideoSender: Камеры не найдены!";
    }
}

VideoSender::~VideoSender() {
    m_workerThread.quit();
    m_workerThread.wait();
}


QVideoSink* VideoSender::sourceSink() const {
    return m_sourceSink;
}

void VideoSender::setSourceSink(QVideoSink* sink) {
    if (m_sourceSink == sink) {
        return;
    }

    if (m_sourceSink) {
        disconnect(m_sourceSink, &QVideoSink::videoFrameChanged, this, &VideoSender::processLocalFrame);
    }

    m_sourceSink = sink;


    if (m_sourceSink) {
        connect(m_sourceSink, &QVideoSink::videoFrameChanged, this, &VideoSender::processLocalFrame);
    }

    emit sourceSinkChanged();
}

bool VideoSender::active() const {
    return m_active;
}

void VideoSender::setActive(bool active) {
    if (m_active == active) {
        return;
    }

    m_active = active;


    if (m_camera) {
        if (m_active) m_camera->start();
        else m_camera->stop();
    }

    emit activeChanged();
}

void VideoSender::processLocalFrame(const QVideoFrame &frame) {
    if (!m_active || !frame.isValid() || m_isWorkerBusy) {
        return;
    }

    m_isWorkerBusy = true;

    QVideoFrame f = frame;

    if (!f.map(QVideoFrame::ReadOnly)) {
        m_isWorkerBusy = false;
        return;
    }

    QImage img = f.toImage();
    f.unmap();

    if (!img.isNull()) {

        img = img.scaled(320, 240, Qt::KeepAspectRatio, Qt::FastTransformation)
                  .convertToFormat(QImage::Format_RGB32);

        emit dispatchImage(img);
    } else {
        m_isWorkerBusy = false;
    }
}
