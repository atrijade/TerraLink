#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/packet/packet.h"
#include "ntn-sim/logger/logger.h"

#include <cmath>
#include <string>

namespace NtnSim::Gateway {

Gateway::Gateway(const std::string& id, const Utils::GeodeticPosition& position,
                 const FeederLinkProfile& profile)
    : m_id(id), m_position(position), m_feederProfile(profile) {}

Gateway::~Gateway() {}

std::string Gateway::GetId() const {
    return m_id;
}

Utils::GeodeticPosition Gateway::GetPosition() const {
    return m_position;
}

// =========================================================================
// Step 1: Feeder Link Profile & Satellite Tracking
// =========================================================================

void Gateway::SetFeederLinkProfile(const FeederLinkProfile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_feederProfile = profile;
}

FeederLinkProfile Gateway::GetFeederLinkProfile() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_feederProfile;
}

bool Gateway::IsSatelliteVisible(double elevationDeg) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return elevationDeg >= m_feederProfile.minElevationDeg;
}

void Gateway::ConnectSatellite(const std::string& satId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connectedSatId = satId;
    m_hasActiveFeederLink = true;
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO,
        "[Gateway:" + m_id + "] Feeder link established with satellite " + satId);
}

void Gateway::DisconnectSatellite() {
    std::lock_guard<std::mutex> lock(m_mutex);
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO,
        "[Gateway:" + m_id + "] Feeder link released from satellite " + m_connectedSatId);
    m_connectedSatId.clear();
    m_hasActiveFeederLink = false;
}

std::string Gateway::GetConnectedSatelliteId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connectedSatId;
}

bool Gateway::HasActiveFeederLink() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hasActiveFeederLink;
}

// =========================================================================
// Step 2: Packet Sink & Statistics
// =========================================================================

bool Gateway::ReceivePacket(uint64_t packetId) {
    return ReceivePacket(packetId, 0.0, 1500, 0.0);
}

bool Gateway::ReceivePacket(std::shared_ptr<Packet::Packet> packet, double timestamp) {
    if (!packet) {
        Logger::Logger::GetInstance().Log(Logger::LogLevel::WARNING,
            "[Gateway:" + m_id + "] Null packet received at t=" + std::to_string(timestamp));
        return false;
    }
    return ReceivePacket(packet->GetId(), timestamp, packet->GetSizeBytes(), packet->GetCreationTime());
}

bool Gateway::ReceivePacket(uint64_t packetId, double timestamp, uint32_t sizeBytes, double creationTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    double transitTime = timestamp - creationTime;
    if (transitTime < 0.0) {
        transitTime = 0.0;
    }

    // 1. Check sequence numbers & detect out-of-order or lost packets
    if (m_hasReceivedAny) {
        if (packetId <= m_lastSequenceNumber) {
            m_stats.outOfOrderPackets++;
        } else if (packetId > m_lastSequenceNumber + 1) {
            m_stats.lostPacketsDetected += (packetId - m_lastSequenceNumber - 1);
        }
        m_lastSequenceNumber = packetId;
    } else {
        m_hasReceivedAny = true;
        m_lastSequenceNumber = packetId;
        m_stats.firstPacketTime = timestamp;
    }

    // 2. Inter-arrival jitter calculation (RFC 3550 smoothed formula)
    if (m_stats.totalPacketsReceived == 0) {
        m_lastTransitTime = transitTime;
        m_stats.currentJitterSeconds = 0.0;
        m_stats.jitterSeconds = 0.0;
    } else {
        double d = std::abs(transitTime - m_lastTransitTime);
        m_stats.currentJitterSeconds = d;
        // RFC 3550 exponential filter: J = J + (|D(i-1, i)| - J) / 16.0
        m_stats.jitterSeconds += (d - m_stats.jitterSeconds) / 16.0;
        m_lastTransitTime = transitTime;
    }

    // 3. Tally counts, byte volume, and latency
    m_stats.totalPacketsReceived++;
    m_stats.totalBytesReceived += sizeBytes;
    m_stats.lastPacketTime = timestamp;
    m_totalLatencySeconds += transitTime;
    m_stats.averageLatencySeconds = m_totalLatencySeconds / static_cast<double>(m_stats.totalPacketsReceived);

    // 4. Tally gateway throughput (Mbps)
    double elapsed = m_stats.lastPacketTime - m_stats.firstPacketTime;
    if (elapsed > 1e-9) {
        m_stats.throughputMbps = (static_cast<double>(m_stats.totalBytesReceived) * 8.0) / (elapsed * 1.0e6);
    } else {
        m_stats.throughputMbps = 0.0;
    }

    // 5. Log packet receipt
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO,
        "[Gateway:" + m_id + "] Received packet #" + std::to_string(packetId) +
        " at t=" + std::to_string(timestamp) + "s (size=" + std::to_string(sizeBytes) +
        " B, latency=" + std::to_string(transitTime * 1000.0) + " ms, jitter=" +
        std::to_string(m_stats.jitterSeconds * 1000.0) + " ms, throughput=" +
        std::to_string(m_stats.throughputMbps) + " Mbps)");

    return true;
}

GatewayStats Gateway::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

uint64_t Gateway::GetTotalPacketsReceived() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.totalPacketsReceived;
}

uint64_t Gateway::GetTotalBytesReceived() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.totalBytesReceived;
}

uint64_t Gateway::GetOutOfOrderPackets() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.outOfOrderPackets;
}

uint64_t Gateway::GetLostPackets() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.lostPacketsDetected;
}

double Gateway::GetJitterSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.jitterSeconds;
}

double Gateway::GetThroughputMbps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.throughputMbps;
}

double Gateway::GetAverageLatencySeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.averageLatencySeconds;
}

void Gateway::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = GatewayStats{};
    m_hasReceivedAny = false;
    m_lastSequenceNumber = 0;
    m_lastTransitTime = 0.0;
    m_totalLatencySeconds = 0.0;
}

} // namespace NtnSim::Gateway
