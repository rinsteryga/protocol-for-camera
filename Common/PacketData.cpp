#include "PacketData.hpp"

#include <cstring>

QByteArray PacketData::toQBA() const
{
    QByteArray packet;
    packet.resize(sizeof(PacketHeader) + payload.size());

    PacketHeader tempHeader = header;
    tempHeader.hash = 0;

    std::memcpy(packet.data(), &tempHeader, sizeof(PacketHeader));

    if (!payload.isEmpty()) {
        std::memcpy(packet.data() + sizeof(PacketHeader), payload.constData(), payload.size());
    }

    quint16 calcHash = qChecksum(packet);

    std::memcpy(packet.data() + offsetof(PacketHeader, hash), &calcHash, sizeof(quint16));

    return packet;
}

std::optional<PacketData> PacketData::fromQBA(const QByteArray &rawData)
{
    if (rawData.size() < static_cast<qsizetype>(sizeof(PacketHeader)))
        return std::nullopt;

    PacketData result;
    std::memcpy(&result.header, rawData.constData(), sizeof(PacketHeader));

    quint16 receivedHash = result.header.hash;

    QByteArray tempPacket = rawData;
    quint16 zeroHash = 0;
    std::memcpy(tempPacket.data() + offsetof(PacketHeader, hash), &zeroHash, sizeof(quint16));

    if (qChecksum(tempPacket) != receivedHash)
        return std::nullopt;

    result.payload = qUncompress(rawData.sliced(sizeof(PacketHeader)));
    return result;
}
