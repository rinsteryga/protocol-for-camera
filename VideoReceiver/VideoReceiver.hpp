#pragma once

#include "PacketData.hpp"

#include <QHostAddress>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QVariant>
#include <QtQml/qqmlregistration.h> // Важно для QML_ELEMENT

#include <unordered_map>

// Размер одного фрагмента данных (должен совпадать с настройками отправителя)
constexpr int CHUNK_SIZE = 1000;

/**
 * @brief Рабочий класс для фонового приема данных по сети.
 */
class ReceiverWorker : public QObject {
    Q_OBJECT
public:
    explicit ReceiverWorker(QObject *parent = nullptr);

public slots:
    /**
     * @brief Инициализирует сокет. На одном ПК используем порт 5556, чтобы не было конфликта с Sender.
     */
    void init();

    /**
     * @brief Отправляет UDP-пакет запроса на указанный IP (порт 5555).
     */
    void sendStartRequest(const QString &ip);

private slots:
    void readPendingDatagrams();

signals:
    void frameAssembled(const QImage &img);

private:
    struct FrameBuffer {
        QByteArray data;
        quint16 chunksReceived = 0;
        quint16 totalChunks = 0;
        quint16 width = 0;
        quint16 height = 0;
    };

    void handleIncomingPacket(const PacketData& packet);

    quint32 m_currentRenderedFrame = 0;
    QUdpSocket* m_udpReceiver = nullptr;
    std::unordered_map<quint32, FrameBuffer> m_frameBuffers;
};

/**
 * @brief Главный класс-менеджер для клиентского приложения (Приемник).
 */
class VideoReceiver : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QImage currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(QString serverIp READ serverIp WRITE setServerIp NOTIFY serverIpChanged)

public:
    explicit VideoReceiver(QObject *parent = nullptr);
    ~VideoReceiver();

    QImage currentFrame() const;

    QString serverIp() const { return m_serverIp; }
    void setServerIp(const QString &ip) {
        if (m_serverIp != ip) {
            m_serverIp = ip;
            emit serverIpChanged();
        }
    }

    /**
     * @brief Метод для вызова из QML (кнопка "Подключиться").
     */
    Q_INVOKABLE void startStreaming();

signals:
    void currentFrameChanged();
    void serverIpChanged();
    void requestStart(const QString &ip);

private slots:
    void onFrameAssembled(const QImage &img);

private:
    QImage m_currentFrame;
    QString m_serverIp;
    QThread m_thread;
    ReceiverWorker* m_worker = nullptr;
};
