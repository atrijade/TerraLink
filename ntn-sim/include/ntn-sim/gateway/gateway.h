#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>

namespace NtnSim::Orbit {
class Satellite;
}

namespace NtnSim::Packet {
class Packet;
class PacketEngine;
}

namespace NtnSim::Signal {
class SignalEngine;
class SignalInfo;
}

namespace NtnSim::Coverage {
class CoverageEngine;
class CoverageInfo;
}

namespace NtnSim::Routing {
class RoutingEngine;
}

namespace NtnSim::Metrics {
class MetricsEngine;
}

namespace NtnSim::Core {
class EventScheduler;
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
 * @brief Ground station parabolic dish pointing vector and geometry.
 */
struct DishPointing {
    double azimuthDeg{0.0};       ///< 0 to 360 deg clockwise from true North
    double elevationDeg{0.0};     ///< -90 to +90 deg relative to local horizon
    double slantRangeMeters{0.0}; ///< Line-of-sight distance to satellite (meters)
    double dopplerShiftHz{0.0};   ///< Carrier frequency Doppler shift (Hz)
    double losVelocityKms{0.0};   ///< Line-of-sight relative velocity (km/s)
    bool isVisible{false};        ///< True if elevationDeg >= minElevationDeg
};

/**
 * @brief Represents an Internet Gateway or satellite Ground Station with 3GPP NTN feeder capabilities.
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
    bool ReceivePacket(uint64_t packetId);
    bool ReceivePacket(uint64_t packetId, double timestamp, uint32_t sizeBytes = 1500, double creationTime = 0.0);
    bool ReceivePacket(std::shared_ptr<Packet::Packet> packet, double timestamp);

    GatewayStats GetStatistics() const;
    uint64_t GetTotalPacketsReceived() const;
    uint64_t GetTotalBytesReceived() const;
    uint64_t GetOutOfOrderPackets() const;
    uint64_t GetLostPackets() const;
    double GetJitterSeconds() const;
    double GetThroughputMbps() const;
    double GetAverageLatencySeconds() const;
    double GetUtilizationPercent() const;
    void ResetStatistics();

    // =========================================================================
    // Step 3 & 4: Feeder Link Handover & Engine Integration
    // =========================================================================

    /**
     * @brief Queries satellite position and computes dish pointing (azimuth, elevation, range, Doppler).
     */
    DishPointing CalculateDishPointing(const Orbit::Satellite& sat) const;

    /**
     * @brief Computes 3GPP NTN feeder link budget (RSRP, SINR, FSPL, Doppler) using SignalEngine.
     */
    Signal::SignalInfo CalculateFeederSignal(const Orbit::Satellite& sat,
                                            const Signal::SignalEngine* signalEngine = nullptr) const;

    /**
     * @brief Selects the best visible feeder satellite (highest elevation angle >= 10 deg).
     */
    std::shared_ptr<Orbit::Satellite> SelectBestFeederSatellite(
        const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const;

    /**
     * @brief Evaluates feeder handover condition. If current satellite approaches the 10 deg mask
     *        (or is lost), switches tracking to the next rising satellite and updates RoutingEngine.
     * @return True if a feeder link handover was executed.
     */
    bool EvaluateFeederHandover(
        const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites,
        Routing::RoutingEngine* routingEngine,
        double currentTime,
        Metrics::MetricsEngine* metricsEngine = nullptr,
        double handoverThresholdDeg = 12.0);

    /**
     * @brief Directly switches feeder tracking to new satellite and updates RoutingEngine graph edges.
     */
    bool SwitchFeederSatellite(
        const std::string& newSatId,
        double distanceMeters,
        Routing::RoutingEngine* routingEngine,
        double currentTime,
        Metrics::MetricsEngine* metricsEngine = nullptr);

    /**
     * @brief Injects an uplink packet from this gateway into the constellation.
     */
    std::shared_ptr<Packet::Packet> InjectUplinkPacket(
        Packet::PacketEngine& packetEngine,
        uint64_t packetId,
        uint32_t sizeBytes,
        double currentTime,
        const std::string& destinationId,
        Core::EventScheduler* scheduler = nullptr);

    /**
     * @brief Records gateway performance telemetry into MetricsEngine.
     */
    void RecordMetrics(Metrics::MetricsEngine* metricsEngine, double currentTime) const;

private:
    double GetUtilizationPercentInternal() const;

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
