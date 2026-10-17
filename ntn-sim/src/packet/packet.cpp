#include "ntn-sim/packet/packet.h"

namespace NtnSim::Packet {

Packet::Packet(uint64_t id, uint32_t sizeBytes, double creationTime, const std::string& srcNodeId, const std::string& destNodeId)
    : m_id(id), m_sizeBytes(sizeBytes), m_creationTime(creationTime), m_srcNodeId(srcNodeId), m_destNodeId(destNodeId) {}

Packet::~Packet() {}

uint64_t Packet::GetId() const {
    return m_id;
}

uint32_t Packet::GetSizeBytes() const {
    return m_sizeBytes;
}

double Packet::GetCreationTime() const {
    return m_creationTime;
}

double Packet::GetArrivalTime() const {
    return m_arrivalTime;
}

void Packet::SetArrivalTime(double time) {
    m_arrivalTime = time;
}

std::string Packet::GetSourceNodeId() const {
    return m_srcNodeId;
}

std::string Packet::GetDestinationNodeId() const {
    return m_destNodeId;
}

} // namespace NtnSim::Packet
