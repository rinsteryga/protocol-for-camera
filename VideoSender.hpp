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

#include <atomic>

/// Размер одного передаваемого фрагмента данных в байтах.
constexpr int CHUNK_SIZE = 1000;

/**
 * @brief Класс для асинхронной передачи видеокадров по сети.
 * * Выполняется в выделенном потоке (Worker Thread). Отвечает за
 * фрагментацию изображений на дейтаграммы заданного размера и
 * их отправку по протоколу UDP.
 */
class SenderWorker : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса SenderWorker.
     * @param parent Указатель на родительский объект QObject.
     */
    explicit SenderWorker(QObject *parent = nullptr);

public slots:
    /**
     * @brief Инициализация сетевых ресурсов.
     * * Создает сокет, привязывает его к системному порту и расширяет
     * размер буфера отправки операционной системы для предотвращения
     * потери пакетов при высокой нагрузке.
     */
    void init();

    /**
     * @brief Обработка и отправка кадра.
     * * Производит расчет необходимого количества фрагментов, разбивает
     * сырой массив пикселей на части, формирует сетевые пакеты и
     * отправляет их целевому узлу.
     * * @param img Изображение, подготовленное для передачи.
     */
    void processImage(QImage img);

signals:
    /**
     * @brief Сигнал, уведомляющий об окончании отправки текущего кадра.
     * * Используется для синхронизации с основным потоком и снятия
     * блокировки на захват следующего кадра.
     */
    void readyForNextFrame();

private:
    QUdpSocket *m_udpSender = nullptr;
    quint32 m_frameCounter = 0;
};

/**
 * @brief Контроллер захвата и управления трансляцией видео.
 * * Функционирует в главном потоке (GUI Thread). Выполняет перехват
 * кадров с устройства записи, их конвертацию в оптимальный формат
 * и диспетчеризацию в рабочий поток для последующей отправки.
 */
class VideoSender : public QObject
{
    Q_OBJECT
    /**
     * @brief Целевой сток видеокадров (QVideoSink), используемый для перехвата потока.
     */
    Q_PROPERTY(QVideoSink *sourceSink READ sourceSink WRITE setSourceSink NOTIFY sourceSinkChanged)

    /**
     * @brief Флаг активности трансляции.
     */
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    /**
     * @brief Конструктор класса VideoSender.
     * @param parent Указатель на родительский объект QObject.
     */
    explicit VideoSender(QObject *parent = nullptr);

    /**
     * @brief Деструктор класса VideoSender.
     * Корректно завершает работу фонового потока перед удалением объекта.
     */
    ~VideoSender();

    QVideoSink *sourceSink() const;
    void setSourceSink(QVideoSink *sink);

    bool active() const;
    void setActive(bool active);

signals:
    void sourceSinkChanged();
    void activeChanged();

    /**
     * @brief Внутренний сигнал для передачи кадра в рабочий поток.
     * @param img Обработанный кадр (масштабированный, в формате RGB32).
     */
    void dispatchImage(QImage img);

private slots:
    /**
     * @brief Слот-обработчик новых кадров, поступающих от локальной камеры.
     * * Выполняет перенос кадра из видеопамяти в оперативную, понижает
     * разрешение до целевого (320x240) и конвертирует цветовое пространство.
     * * @param frame Сырой кадр, полученный от QVideoSink.
     */
    void processLocalFrame(const QVideoFrame &frame);

private:
    QVideoSink *m_sourceSink = nullptr;
    bool m_active = false;

    /// Атомарный флаг состояния занятости фонового потока (Drop-frame механизм).
    std::atomic<bool> m_isWorkerBusy{false};

    QThread m_thread;
    SenderWorker *m_worker = nullptr;
};
