/**
 * @file PacketHeader.hpp
 * @brief Описание структур данных для сетевого протокола передачи видео.
 */
#pragma once

#include <QtGlobal>

/**
 * @brief Типы передаваемых сетевых сообщений.
 */
enum class MsgType : quint8
{
    VideoFrame = 1, ///< Пакет содержит фрагмент видеокадра
    Request = 2     ///< Служебный запрос (зарезервировано для расширения)
};

/**
 * @brief Заголовок сетевого пакета.
 * Упакованная структура (без выравнивания байтов), содержащая метаданные фрагмента.
 */
struct __attribute__((packed)) PacketHeader
{
    MsgType type;           ///< Идентификатор типа сообщения
    quint16 hash;           ///< Поле для контрольной суммы
    quint32 frameId;        ///< Глобальный инкрементальный идентификатор кадра
    quint16 fragmentIdx;    ///< Порядковый индекс текущего фрагмента в кадре
    quint16 totalFragments; ///< Общее количество фрагментов, составляющих кадр
    quint16 imgWidth;       ///< Ширина кадра в пикселях
    quint16 imgHeight;      ///< Высота кадра в пикселях
};

static_assert(sizeof(PacketHeader) == 15, "PacketHeader size mismatch");
