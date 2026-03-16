#pragma once

#include "PacketData.hpp"
#include <QHostAddress>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QVariant>
#include <QVideoFrame>
#include <QVideoSink>
#include <QMediaCaptureSession>
#include <QCamera>
#include <QMediaDevices>

#include <atomic>

/**
 * @constant CHUNK_SIZE
 * @brief Максимальный размер полезной нагрузки (payload) в одном UDP-пакете.
 * Выбран исходя из ограничений MTU для минимизации фрагментации на сетевом уровне.
 */
constexpr int CHUNK_SIZE = 1000;

/**
 * @class SenderWorker
 * @brief Сетевой агент для асинхронной фрагментации и отправки данных.
 * * Класс инкапсулирует логику работы с UDP-сокетом в отдельном потоке.
 * Реализует механизм динамического определения адреса клиента (Handshake)
 * и последовательную отправку фрагментов видеокадра.
 */
class SenderWorker : public QObject
{
    Q_OBJECT
public:
    explicit SenderWorker(QObject *parent = nullptr);

public slots:
    /** @brief Инициализация сокета и настройка системных буферов. */
    void init();

    /** * @brief Обработка и отправка изображения.
     * Выполняет сжатие, расчет контрольных сумм и циклическую отправку датаграмм.
     */
    void processImage(QImage img);

    /** @brief Обработчик входящих UDP-пакетов для инициализации сессии (Handshake). */
    void readPendingDatagrams();

signals:
    /** @brief Сигнал готовности к обработке следующего кадра (разблокировка Flow Control). */
    void readyForNextFrame();

private:
    QUdpSocket *m_udpSender = nullptr;
    quint32 m_frameCounter = 0;

    QHostAddress m_targetAddress; ///< Динамически определенный IP-адрес клиента.
    quint16 m_targetPort = 0;      ///< Динамически определенный порт клиента.
    bool m_isStreaming = false;    ///< Флаг активной сессии трансляции.
};

/**
 * @class VideoSender
 * @brief Фасад управления видеотрансляцией и захватом.
 * * Объединяет механизмы захвата Qt Multimedia и логику сетевого потока.
 * Отвечает за предварительную обработку кадров (Scale, Format Conversion)
 * и управление жизненным циклом фонового потока (Worker Thread).
 */
class VideoSender : public QObject
{
    Q_OBJECT

    /** @property sourceSink Приемник видеопотока для захвата кадров. */
    Q_PROPERTY(QVideoSink *sourceSink READ sourceSink WRITE setSourceSink NOTIFY sourceSinkChanged)

    /** @property active Состояние активности вещания. */
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    explicit VideoSender(QObject *parent = nullptr);

    /** @brief Деструктор с безопасной остановкой потока отправки. */
    ~VideoSender();

    QVideoSink *sourceSink() const;
    void setSourceSink(QVideoSink *sink);

    bool active() const;
    void setActive(bool active);

signals:
    void sourceSinkChanged();
    void activeChanged();

    /** * @brief Передача подготовленного изображения в поток отправки.
     * @param img Изображение в формате RGB32, готовое к сериализации.
     */
    void dispatchImage(QImage img);

private slots:
    /** * @brief Конвейер обработки сырых кадров.
     * Выполняет Map-инг видеопамяти, конвертацию цветового пространства
     * и проверку занятости сетевого воркера (Drop-frame logic).
     */
    void processLocalFrame(const QVideoFrame &frame);

private:
    QVideoSink *m_sourceSink = nullptr;
    bool m_active = false;

    /** * @brief Флаг синхронизации потоков.
     * Предотвращает накопление очереди кадров при низкой пропускной способности сети.
     */
    std::atomic<bool> m_isWorkerBusy{false};

    QThread m_workerThread;    ///< Выделенный поток для сетевых операций.
    SenderWorker *m_worker = nullptr;

           // Компоненты для работы захвата в Headless-режиме (без QML)
    QCamera* m_camera = nullptr;
    QMediaCaptureSession* m_captureSession = nullptr;
};
