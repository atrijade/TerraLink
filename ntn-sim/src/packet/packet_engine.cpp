#include "ntn-sim/packet/packet_engine.h"

namespace NtnSim::Packet {

PacketEngine::PacketEngine() {}

PacketEngine::~PacketEngine() {}

std::shared_ptr<Packet> PacketEngine::GeneratePacket(uint64_t packetId, uint32_t sizeBytes, double creationTime, const std::string& srcNodeId, const std::string& destNodeId) {
    auto packet = std::make_shared<Packet>(packetId, sizeBytes, creationTime, srcNodeId, destNodeId);
    return packet;
}

void PacketEngine::EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues[queueId].push(packet);
}

std::shared_ptr<Packet> PacketEngine::DequeuePacket(const std::string& queueId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end() && !it->second.empty()) {
        auto pkt = it->second.front();
        it->second.pop();
        return pkt;
    }
    return nullptr;
}

size_t PacketEngine::GetQueueSize(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.size();
    }
    return 0;
}

} // namespace NtnSim::Packet
