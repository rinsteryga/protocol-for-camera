#include "PacketData.hpp"

#include <cstring>

QByteArray PacketData::toQBA() const {
    // Сжимаем ТОЛЬКО данные
    QByteArray compressedPayload = qCompress(payload, 5);

    QByteArray packet;
    packet.resize(sizeof(PacketHeader) + compressedPayload.size());

    PacketHeader tempHeader = header;
    tempHeader.hash = 0;

           // Копируем заголовок (сырой) и данные (сжатые)
    std::memcpy(packet.data(), &tempHeader, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), compressedPayload.constData(), compressedPayload.size());

           // Хеш считаем от всего готового "бутерброда"
    quint16 calcHash = qChecksum(packet);
    std::memcpy(packet.data() + offsetof(PacketHeader, hash), &calcHash, sizeof(quint16));

    return packet;
}

std::optional<PacketData> PacketData::fromQBA(const QByteArray &rawData) {
    if (rawData.size() < (qsizetype)sizeof(PacketHeader)) return std::nullopt;

           // 1. Сначала проверяем хеш (целостность пакета как он есть)
           // ... (код проверки хеша, который у тебя был) ...

    PacketData result;
    std::memcpy(&result.header, rawData.constData(), sizeof(PacketHeader));

           // 2. Распаковываем ТОЛЬКО то, что идет после заголовка
    QByteArray compressedPart = rawData.mid(sizeof(PacketHeader));
    if (!compressedPart.isEmpty()) {
        result.payload = qUncompress(compressedPart);
    }

    return result;
}
