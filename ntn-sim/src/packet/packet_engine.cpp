#include "ntn-sim/packet/packet_engine.h"

namespace NtnSim::Packet {

PacketEngine::PacketEngine() {}

PacketEngine::~PacketEngine() {}

std::shared_ptr<Packet> PacketEngine::GeneratePacket(uint64_t packetId, uint32_t sizeBytes,
                                                    double creationTime, const std::string& srcNodeId,
                                                    const std::string& destNodeId,
                                                    uint32_t qosClass, uint32_t ttl,
                                                    double maxLifetimeSeconds) {
    return std::make_shared<Packet>(packetId, sizeBytes, creationTime, srcNodeId, destNodeId, qosClass, ttl, maxLifetimeSeconds);
}

void PacketEngine::ConfigureQueue(const std::string& queueId, size_t maxPackets, size_t maxSizeBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& q = m_queues[queueId];
    q.config.maxPackets = maxPackets;
    q.config.maxSizeBytes = maxSizeBytes;
}

void PacketEngine::ConfigureQueue(const std::string& queueId, const QueueConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues[queueId].config = config;
}

QueueConfig PacketEngine::GetQueueConfig(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.config;
    }
    return QueueConfig{};
}

bool PacketEngine::EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet, double enqueueTime) {
    if (!packet) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto& q = m_queues[queueId];

    // Check DropTail limits (packet count and byte limits)
    bool packetLimitExceeded = (q.packets.size() >= q.config.maxPackets);
    bool byteLimitExceeded = (q.config.maxSizeBytes > 0 &&
                             (q.currentSizeBytes + packet->GetSizeBytes() > q.config.maxSizeBytes));

    if (packetLimitExceeded || byteLimitExceeded) {
        q.droppedPacketsCount++;
        return false; // Dropped due to buffer overflow
    }

    packet->SetEnqueueTime(enqueueTime);
    q.packets.push(packet);
    q.currentSizeBytes += packet->GetSizeBytes();
    return true;
}

std::shared_ptr<Packet> PacketEngine::DequeuePacket(const std::string& queueId, double dequeueTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it == m_queues.end() || it->second.packets.empty()) {
        return nullptr;
    }

    auto packet = it->second.packets.front();
    it->second.packets.pop();

    if (it->second.currentSizeBytes >= packet->GetSizeBytes()) {
        it->second.currentSizeBytes -= packet->GetSizeBytes();
    } else {
        it->second.currentSizeBytes = 0;
    }

    packet->SetDepartureTime(dequeueTime);
    double delay = dequeueTime - packet->GetEnqueueTime();
    it->second.lastQueueDelaySeconds = (delay >= 0.0) ? delay : 0.0;

    return packet;
}

size_t PacketEngine::GetQueueSize(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.packets.size();
    }
    return 0;
}

size_t PacketEngine::GetQueueSizeBytes(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.currentSizeBytes;
    }
    return 0;
}

size_t PacketEngine::GetDroppedPacketCount(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.droppedPacketsCount;
    }
    return 0;
}

double PacketEngine::GetLastQueueDelay(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.lastQueueDelaySeconds;
    }
    return 0.0;
}

void PacketEngine::ClearQueue(const std::string& queueId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        while (!it->second.packets.empty()) {
            it->second.packets.pop();
        }
        it->second.currentSizeBytes = 0;
    }
}

} // namespace NtnSim::Packet
