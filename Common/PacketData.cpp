#include "PacketData.hpp"

#include <cstring>

QByteArray PacketData::toQBA() const {
    QByteArray compressedPayload = qCompress(payload, 5);

    QByteArray packet;
    packet.resize(sizeof(PacketHeader) + compressedPayload.size());

    PacketHeader tempHeader = header;
    tempHeader.hash = 0;

    std::memcpy(packet.data(), &tempHeader, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), compressedPayload.constData(), compressedPayload.size());

    quint16 calcHash = qChecksum(packet);
    std::memcpy(packet.data() + offsetof(PacketHeader, hash), &calcHash, sizeof(quint16));

    return packet;
}

std::optional<PacketData> PacketData::fromQBA(const QByteArray &rawData) {
    if (rawData.size() < (qsizetype)sizeof(PacketHeader)) return std::nullopt;

    PacketData result;
    std::memcpy(&result.header, rawData.constData(), sizeof(PacketHeader));

    QByteArray compressedPart = rawData.mid(sizeof(PacketHeader));
    if (!compressedPart.isEmpty()) {
        result.payload = qUncompress(compressedPart);
    }

    return result;
}
