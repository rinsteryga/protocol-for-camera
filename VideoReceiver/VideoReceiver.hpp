#pragma once

#include "PacketData.hpp"

#include <QHostAddress>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QVariant>

#include <unordered_map>

// Размер одного фрагмента данных (должен совпадать с настройками отправителя)
constexpr int CHUNK_SIZE = 1000;

/**
 * @brief Рабочий класс для фонового приема данных по сети.
 * Выполняется в отдельном потоке, чтобы не блокировать графический интерфейс (GUI).
 * Принимает датаграммы, валидирует их и склеивает фрагменты обратно в QImage.
 */
class ReceiverWorker : public QObject {
    Q_OBJECT
public:
    explicit ReceiverWorker(QObject *parent = nullptr);

public slots:
    /**
     * @brief Инициализирует сокет и начинает прослушивание порта.
     * Должен вызываться после старта рабочего потока.
     */
    void init();

private slots:
    /**
     * @brief Слот, срабатывающий при поступлении новых данных на UDP-порт.
     * Считывает все доступные датаграммы и передает их на парсинг.
     */
    void readPendingDatagrams();

signals:
    /**
     * @brief Сигнал об успешной сборке целого кадра (или кадра с допустимыми потерями).
     * @param img Готовое изображение.
     */
    void frameAssembled(const QImage &img);

private:
    /**
     * @brief Внутренняя структура для накопления фрагментов одного кадра.
     */
    struct FrameBuffer {
        QByteArray data;
        quint16 chunksReceived = 0;
        quint16 totalChunks = 0;
        quint16 width = 0;
        quint16 height = 0;
    };

    /**
     * @brief Обрабатывает распакованный пакет и помещает его в буфер сборки.
     * Если получен кусок от нового кадра, принудительно завершает сборку старого.
     * @param packet Десериализованный пакет с данными.
     */
    void handleIncomingPacket(const PacketData& packet);

    quint32 m_currentRenderedFrame = 0;
    QUdpSocket* m_udpReceiver = nullptr;
    std::unordered_map<quint32, FrameBuffer> m_frameBuffers;
};

/**
 * @brief Главный класс-менеджер для клиентского приложения (Приемник).
 * Живет в главном потоке. Управляет фоновым Worker'ом и пробрасывает собранные кадры
 * в графический интерфейс QML через систему свойств (Q_PROPERTY).
 */
class VideoReceiver : public QObject {
    Q_OBJECT
    /** * @brief Свойство, хранящее текущий кадр.
     * Доступно для чтения из QML, при обновлении генерирует сигнал currentFrameChanged.
     */
    Q_PROPERTY(QImage currentFrame READ currentFrame NOTIFY currentFrameChanged)

public:
    explicit VideoReceiver(QObject *parent = nullptr);
    ~VideoReceiver();

    /**
     * @brief Возвращает текущий собранный кадр.
     */
    QImage currentFrame() const;

signals:
    /**
     * @brief Сигнал об изменении текущего кадра (уведомляет QML о необходимости перерисовки).
     */
    void currentFrameChanged();

private slots:
    /**
     * @brief Принимает собранный кадр из рабочего потока.
     * @param img Изображение, готовое к выводу.
     */
    void onFrameAssembled(const QImage &img);

private:
    QImage m_currentFrame;
    QThread m_thread;
    ReceiverWorker* m_worker = nullptr;
};
