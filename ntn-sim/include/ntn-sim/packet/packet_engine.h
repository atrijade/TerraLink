#pragma once

#include "ntn-sim/packet/packet.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

namespace NtnSim::Packet {

/**
 * @brief Manages queuing, traffic generation, buffer sizes, and delivery status.
 */
class PacketEngine {
public:
    PacketEngine();
    ~PacketEngine();

    /**
     * @brief Creates and enqueues a new packet.
     */
    std::shared_ptr<Packet> GeneratePacket(uint64_t packetId, uint32_t sizeBytes, double creationTime, const std::string& srcNodeId, const std::string& destNodeId);

    /**
     * @brief Places a packet into a queue.
     */
    void EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet);

    /**
     * @brief Pops a packet from a queue.
     */
    std::shared_ptr<Packet> DequeuePacket(const std::string& queueId);

    /**
     * @brief Gets current queue length.
     */
    size_t GetQueueSize(const std::string& queueId) const;

private:
    std::unordered_map<std::string, std::queue<std::shared_ptr<Packet>>> m_queues;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Packet
