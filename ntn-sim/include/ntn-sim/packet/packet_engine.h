#pragma once

#include "ntn-sim/packet/packet.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <cstddef>
#include <cstdint>

namespace NtnSim::Packet {

/**
 * @brief Configuration parameters for a node transmission queue.
 */
struct QueueConfig {
    size_t maxPackets{1000};                     ///< Maximum packet capacity before DropTail
    size_t maxSizeBytes{10 * 1024 * 1024};       ///< Maximum byte capacity (default 10 MB)
};

/**
 * @brief Dynamic runtime state for a node packet buffer.
 */
struct QueueState {
    QueueConfig config;
    std::queue<std::shared_ptr<Packet>> packets;
    size_t currentSizeBytes{0};
    size_t droppedPacketsCount{0};
    double lastQueueDelaySeconds{0.0};
};

/**
 * @brief Manages queuing, traffic generation, buffer limits, and DropTail policies.
 */
class PacketEngine {
public:
    PacketEngine();
    ~PacketEngine();

    /**
     * @brief Creates a new network packet PDU.
     */
    std::shared_ptr<Packet> GeneratePacket(uint64_t packetId, uint32_t sizeBytes,
                                          double creationTime, const std::string& srcNodeId,
                                          const std::string& destNodeId,
                                          uint32_t qosClass = 2, uint32_t ttl = 64,
                                          double maxLifetimeSeconds = 2.0);

    /**
     * @brief Configures buffer capacity limits for a specific queue.
     */
    void ConfigureQueue(const std::string& queueId, size_t maxPackets, size_t maxSizeBytes = 10 * 1024 * 1024);

    /**
     * @brief Configures buffer capacity limits using a QueueConfig struct.
     */
    void ConfigureQueue(const std::string& queueId, const QueueConfig& config);

    /**
     * @brief Gets current configuration of a queue.
     */
    QueueConfig GetQueueConfig(const std::string& queueId) const;

    /**
     * @brief Enqueues a packet into a node queue.
     *        Applies DropTail policy if queue limit is exceeded.
     * @param queueId Node queue identifier.
     * @param packet Packet to enqueue.
     * @param enqueueTime Simulation timestamp when packet entered queue.
     * @return true if enqueued successfully, false if dropped due to buffer overflow.
     */
    bool EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet, double enqueueTime = 0.0);

    /**
     * @brief Pops the front packet from a queue and calculates queue latency.
     * @param queueId Node queue identifier.
     * @param dequeueTime Simulation timestamp when packet departed queue.
     * @return Dequeued packet, or nullptr if queue is empty.
     */
    std::shared_ptr<Packet> DequeuePacket(const std::string& queueId, double dequeueTime = 0.0);

    /**
     * @brief Gets current number of packets in queue.
     */
    size_t GetQueueSize(const std::string& queueId) const;

    /**
     * @brief Gets current size of queued packets in bytes.
     */
    size_t GetQueueSizeBytes(const std::string& queueId) const;

    /**
     * @brief Gets total number of packets dropped by DropTail policy in this queue.
     */
    size_t GetDroppedPacketCount(const std::string& queueId) const;

    /**
     * @brief Gets the queue latency (t_dequeue - t_enqueue) of the most recently dequeued packet.
     */
    double GetLastQueueDelay(const std::string& queueId) const;

    /**
     * @brief Clears all packets from a specific queue.
     */
    void ClearQueue(const std::string& queueId);

private:
    std::unordered_map<std::string, QueueState> m_queues;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Packet
