#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>

namespace NtnSim::Packet {
class Packet;
}

namespace NtnSim::Gateway {

/**
 * @brief Realistic 3GPP NTN Feeder Link profile specifications.
 *        Typically Ka-band (28 GHz) or Q/V-band (40 GHz) feeder links with high bandwidth.
 */
struct FeederLinkProfile {
    double carrierFrequencyHz{28.0e9};     ///< 28 GHz (Ka-band) or 40 GHz (Q/V-band)
    double bandwidthHz{500.0e6};           ///< 500 MHz - 1 GHz
    double dataRateBps{2.5e9};             ///< 1 Gbps - 10 Gbps (e.g. 2.5 Gbps or 10 Gbps)
    double minElevationDeg{10.0};          ///< Minimum elevation mask (10.0 deg) to clear terrain clutter
    double txPowerDbm{43.0};               ///< High-power ground station amplifier (EIRP / Tx power)
    double antennaGainDbi{45.0};           ///< High-gain dish antenna (~1-2m parabolic dish)

    static FeederLinkProfile KaBand() {
        FeederLinkProfile p;
        p.carrierFrequencyHz = 28.0e9;
        p.bandwidthHz = 500.0e6;
        p.dataRateBps = 2.5e9;
        p.minElevationDeg = 10.0;
        p.txPowerDbm = 43.0;
        p.antennaGainDbi = 45.0;
        return p;
    }

    static FeederLinkProfile QvBand() {
        FeederLinkProfile p;
        p.carrierFrequencyHz = 40.0e9;
        p.bandwidthHz = 1.0e9;
        p.dataRateBps = 10.0e9;
        p.minElevationDeg = 10.0;
        p.txPowerDbm = 46.0;
        p.antennaGainDbi = 48.0;
        return p;
    }
};

/**
 * @brief Gateway packet reception telemetry and performance statistics.
 */
struct GatewayStats {
    uint64_t totalPacketsReceived{0};
    uint64_t totalBytesReceived{0};
    uint64_t outOfOrderPackets{0};
    uint64_t lostPacketsDetected{0};
    double firstPacketTime{0.0};
    double lastPacketTime{0.0};
    double averageLatencySeconds{0.0};
    double currentJitterSeconds{0.0};      ///< Instantaneous inter-arrival variation |D_i - D_{i-1}|
    double jitterSeconds{0.0};             ///< RFC 3550 smoothed inter-arrival jitter
    double throughputMbps{0.0};            ///< Cumulative throughput (total bits / elapsed time)
};

/**
 * @brief Represents an Internet Gateway or satellite Ground Station.
 */
class Gateway {
public:
    /**
     * @brief Constructs a new Gateway component.
     * @param id Unique gateway identifier.
     * @param position Geodetic position on Earth.
     * @param profile Feeder link specifications.
     */
    Gateway(const std::string& id, const Utils::GeodeticPosition& position,
            const FeederLinkProfile& profile = FeederLinkProfile::KaBand());
    ~Gateway();

    std::string GetId() const;
    Utils::GeodeticPosition GetPosition() const;

    // =========================================================================
    // Step 1: Feeder Link Profile & Satellite Tracking
    // =========================================================================
    void SetFeederLinkProfile(const FeederLinkProfile& profile);
    FeederLinkProfile GetFeederLinkProfile() const;

    bool IsSatelliteVisible(double elevationDeg) const;
    void ConnectSatellite(const std::string& satId);
    void DisconnectSatellite();
    std::string GetConnectedSatelliteId() const;
    bool HasActiveFeederLink() const;

    // =========================================================================
    // Step 2: Packet Sink & Statistics
    // =========================================================================
    /**
     * @brief Receives a packet by ID (legacy / basic overload).
     */
    bool ReceivePacket(uint64_t packetId);

    /**
     * @brief Receives a packet with explicit metadata for sink telemetry.
     */
    bool ReceivePacket(uint64_t packetId, double timestamp, uint32_t sizeBytes = 1500, double creationTime = 0.0);

    /**
     * @brief Receives a full Packet object downlinked from satellite.
     */
    bool ReceivePacket(std::shared_ptr<Packet::Packet> packet, double timestamp);

    GatewayStats GetStatistics() const;
    uint64_t GetTotalPacketsReceived() const;
    uint64_t GetTotalBytesReceived() const;
    uint64_t GetOutOfOrderPackets() const;
    uint64_t GetLostPackets() const;
    double GetJitterSeconds() const;
    double GetThroughputMbps() const;
    double GetAverageLatencySeconds() const;
    void ResetStatistics();

private:
    std::string m_id;
    Utils::GeodeticPosition m_position;
    FeederLinkProfile m_feederProfile;

    std::string m_connectedSatId;
    bool m_hasActiveFeederLink{false};

    mutable std::mutex m_mutex;
    GatewayStats m_stats;
    bool m_hasReceivedAny{false};
    uint64_t m_lastSequenceNumber{0};
    double m_lastTransitTime{0.0};
    double m_totalLatencySeconds{0.0};
};

} // namespace NtnSim::Gateway
