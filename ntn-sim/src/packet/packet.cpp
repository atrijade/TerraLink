#include "ntn-sim/packet/packet.h"

namespace NtnSim::Packet {

Packet::Packet(uint64_t id, uint32_t sizeBytes, double creationTime,
               const std::string& srcNodeId, const std::string& destNodeId,
               uint32_t qosClass, uint32_t ttl, double maxLifetimeSeconds)
    : m_id(id),
      m_sizeBytes(sizeBytes),
      m_srcNodeId(srcNodeId),
      m_destNodeId(destNodeId),
      m_ttl(ttl),
      m_maxLifetimeSeconds(maxLifetimeSeconds),
      m_qosClass(qosClass),
      m_creationTime(creationTime) {
    m_hopHistory.push_back(srcNodeId);
}

Packet::~Packet() {}

uint64_t Packet::GetId() const {
    return m_id;
}

uint32_t Packet::GetSizeBytes() const {
    return m_sizeBytes;
}

std::string Packet::GetSourceNodeId() const {
    return m_srcNodeId;
}

std::string Packet::GetDestinationNodeId() const {
    return m_destNodeId;
}

uint32_t Packet::GetTtl() const {
    return m_ttl;
}

void Packet::SetTtl(uint32_t ttl) {
    m_ttl = ttl;
}

bool Packet::DecrementTtl() {
    if (m_ttl > 0) {
        m_ttl--;
    }
    return m_ttl > 0;
}

double Packet::GetMaxLifetimeSeconds() const {
    return m_maxLifetimeSeconds;
}

void Packet::SetMaxLifetimeSeconds(double maxLifetime) {
    m_maxLifetimeSeconds = maxLifetime;
}

bool Packet::IsExpired(double currentTime) const {
    if (m_ttl == 0) {
        return true;
    }
    if (m_maxLifetimeSeconds > 0.0 && (currentTime - m_creationTime) > m_maxLifetimeSeconds) {
        return true;
    }
    return false;
}

uint32_t Packet::GetQosClass() const {
    return m_qosClass;
}

void Packet::SetQosClass(uint32_t qosClass) {
    m_qosClass = qosClass;
}

void Packet::AddHop(const std::string& nodeId) {
    m_hopHistory.push_back(nodeId);
}

std::vector<std::string> Packet::GetHopHistory() const {
    return m_hopHistory;
}

size_t Packet::GetHopCount() const {
    return m_hopHistory.empty() ? 0 : (m_hopHistory.size() - 1);
}

double Packet::GetCreationTime() const {
    return m_creationTime;
}

double Packet::GetEnqueueTime() const {
    return m_enqueueTime;
}

void Packet::SetEnqueueTime(double time) {
    m_enqueueTime = time;
}

double Packet::GetDepartureTime() const {
    return m_departureTime;
}

void Packet::SetDepartureTime(double time) {
    m_departureTime = time;
}

double Packet::GetArrivalTime() const {
    return m_arrivalTime;
}

void Packet::SetArrivalTime(double time) {
    m_arrivalTime = time;
}

double Packet::GetQueueDelay() const {
    return (m_departureTime >= m_enqueueTime) ? (m_departureTime - m_enqueueTime) : 0.0;
}

double Packet::GetEndToEndLatency() const {
    return (m_arrivalTime >= m_creationTime) ? (m_arrivalTime - m_creationTime) : 0.0;
}

} // namespace NtnSim::Packet
