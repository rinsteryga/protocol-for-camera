#include "PacketData.hpp"

#include <cstring>

QByteArray PacketData::toQBA() const
{
    // 1. Сжимаем только payload
    QByteArray compressedPayload = qCompress(payload, 9);

    QByteArray packet;
    // Размер теперь: заголовок + сжатые данные
    packet.resize(sizeof(PacketHeader) + compressedPayload.size());

    PacketHeader tempHeader = header;
    tempHeader.hash = 0;
    // Обновляем размер в заголовке, чтобы получатель знал длину сжатых данных
    // (Если в заголовке есть поле size, если нет — не страшно, определим по размеру датаграммы)

           // 2. Копируем заголовок
    std::memcpy(packet.data(), &tempHeader, sizeof(PacketHeader));

           // 3. Копируем СЖАТЫЕ данные
    std::memcpy(packet.data() + sizeof(PacketHeader), compressedPayload.constData(), compressedPayload.size());

           // 4. Считаем хеш от всего готового пакета
    quint16 calcHash = qChecksum(packet);
    std::memcpy(packet.data() + offsetof(PacketHeader, hash), &calcHash, sizeof(quint16));

    return packet;
}

std::optional<PacketData> PacketData::fromQBA(const QByteArray &rawData)
{
    if (rawData.size() < static_cast<qsizetype>(sizeof(PacketHeader)))
        return std::nullopt;

    quint16 receivedHash;
    std::memcpy(&receivedHash, rawData.constData() + offsetof(PacketHeader, hash), sizeof(quint16));

    QByteArray tempPacket = rawData;
    quint16 zeroHash = 0;
    std::memcpy(tempPacket.data() + offsetof(PacketHeader, hash), &zeroHash, sizeof(quint16));

    if (qChecksum(tempPacket) != receivedHash) {
        return std::nullopt; // Данные побились при передаче
    }

           // 2. Читаем заголовок
    PacketData result;
    std::memcpy(&result.header, rawData.constData(), sizeof(PacketHeader));

           // 3. Извлекаем и РАСПАКОВЫВАЕМ payload
    QByteArray compressedData = rawData.mid(sizeof(PacketHeader));
    if (!compressedData.isEmpty()) {
        result.payload = qUncompress(compressedData);
        if (result.payload.isEmpty() && !compressedData.isEmpty()) {
            return std::nullopt; // Ошибка распаковки
        }
    }

    return result;
}
