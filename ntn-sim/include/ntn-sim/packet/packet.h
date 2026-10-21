#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace NtnSim::Packet {

/**
 * @brief Model representing a networking packet PDU (Protocol Data Unit) in the constellation.
 */
class Packet {
public:
    Packet(uint64_t id, uint32_t sizeBytes, double creationTime,
           const std::string& srcNodeId, const std::string& destNodeId,
           uint32_t qosClass = 2, uint32_t ttl = 64, double maxLifetimeSeconds = 2.0);
    ~Packet();

    // Basic Identification
    uint64_t GetId() const;
    uint32_t GetSizeBytes() const;
    std::string GetSourceNodeId() const;
    std::string GetDestinationNodeId() const;

    // Header Metadata (Phase 1)
    uint32_t GetTtl() const;
    void SetTtl(uint32_t ttl);
    bool DecrementTtl();

    double GetMaxLifetimeSeconds() const;
    void SetMaxLifetimeSeconds(double maxLifetime);
    bool IsExpired(double currentTime) const;

    uint32_t GetQosClass() const;
    void SetQosClass(uint32_t qosClass);

    void AddHop(const std::string& nodeId);
    std::vector<std::string> GetHopHistory() const;
    size_t GetHopCount() const;

    // Timing Timestamps (Phase 1)
    double GetCreationTime() const;

    double GetEnqueueTime() const;
    void SetEnqueueTime(double time);

    double GetDepartureTime() const;
    void SetDepartureTime(double time);

    double GetArrivalTime() const;
    void SetArrivalTime(double time);

    double GetQueueDelay() const;
    double GetEndToEndLatency() const;

private:
    uint64_t m_id;
    uint32_t m_sizeBytes;
    std::string m_srcNodeId;
    std::string m_destNodeId;

    // Header metadata
    uint32_t m_ttl{64};
    double m_maxLifetimeSeconds{2.0};
    uint32_t m_qosClass{2}; // 0 = Control/Signaling, 1 = Real-time voice/video, 2 = Best-effort data
    std::vector<std::string> m_hopHistory;

    // Timestamps
    double m_creationTime{0.0};
    double m_enqueueTime{0.0};
    double m_departureTime{0.0};
    double m_arrivalTime{0.0};
};

} // namespace NtnSim::Packet
