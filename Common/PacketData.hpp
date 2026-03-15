/**
 * @file PacketData.hpp
 * @brief Объявление класса для сериализации и десериализации сетевых датаграмм.
 */
#pragma once

#include "PacketHeader.hpp"

#include <QByteArray>
#include <QtGlobal>

#include <optional>

/**
 * @brief Класс-контейнер для полезной нагрузки сетевого пакета.
 * Предоставляет статические и константные методы для преобразования данных
 * в бинарный формат, пригодный для передачи по протоколу UDP, и обратно.
 */
class PacketData
{
public:
    /**
     * @brief Сериализация пакета в массив байтов.
     * @return Бинарный массив, включающий контрольную сумму, заголовок и полезную нагрузку.
     */
    QByteArray toQBA() const;

    /**
     * @brief Десериализация бинарного массива в объект пакета.
     * @param rawData Сырые данные, полученные из сетевого сокета.
     * @return Объект PacketData, либо std::nullopt в случае повреждения данных или несовпадения хэша.
     */
    static std::optional<PacketData> fromQBA(const QByteArray &rawData);

    PacketHeader header{}; ///< Экземпляр заголовка пакета
    QByteArray payload;    ///< Полезная нагрузка (сырые байты фрагмента изображения)
};
