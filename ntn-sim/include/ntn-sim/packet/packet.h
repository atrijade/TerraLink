#pragma once

#include <string>

namespace NtnSim::Packet {

/**
 * @brief Model representing a networking packet transmitted in the constellation.
 */
class Packet {
public:
    Packet(uint64_t id, uint32_t sizeBytes, double creationTime, const std::string& srcNodeId, const std::string& destNodeId);
    ~Packet();

    uint64_t GetId() const;
    uint32_t GetSizeBytes() const;
    
    double GetCreationTime() const;
    
    double GetArrivalTime() const;
    void SetArrivalTime(double time);

    std::string GetSourceNodeId() const;
    std::string GetDestinationNodeId() const;

private:
    uint64_t m_id;
    uint32_t m_sizeBytes;
    double m_creationTime;
    double m_arrivalTime{0.0};
    std::string m_srcNodeId;
    std::string m_destNodeId;
};

} // namespace NtnSim::Packet
