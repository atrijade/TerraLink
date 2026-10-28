#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/packet/packet.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/signal_info.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/coverage/coverage_info.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/logger/logger.h"

#include <cmath>
#include <algorithm>
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

double Gateway::GetUtilizationPercent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return GetUtilizationPercentInternal();
}

double Gateway::GetUtilizationPercentInternal() const {
    double maxMbps = m_feederProfile.dataRateBps / 1.0e6;
    if (maxMbps <= 0.0) {
        return 0.0;
    }
    double util = (m_stats.throughputMbps / maxMbps) * 100.0;
    return std::clamp(util, 0.0, 100.0);
}

void Gateway::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = GatewayStats{};
    m_hasReceivedAny = false;
    m_lastSequenceNumber = 0;
    m_lastTransitTime = 0.0;
    m_totalLatencySeconds = 0.0;
}

// =========================================================================
// Step 3 & 4: Feeder Link Handover & Engine Integration
// =========================================================================

DishPointing Gateway::CalculateDishPointing(const Orbit::Satellite& sat) const {
    DishPointing pointing;

    // 1. Gateway ECEF
    Utils::Position3D gsEcef = Utils::MathUtils::GeodeticToEcef(m_position);

    // 2. Satellite ECEF
    Utils::Position3D satEcef = sat.GetOrbitState().GetEcefPosition();

    // 3. Slant range vector
    double dx = satEcef.x - gsEcef.x;
    double dy = satEcef.y - gsEcef.y;
    double dz = satEcef.z - gsEcef.z;
    double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    pointing.slantRangeMeters = dist;

    if (dist <= 0.0) {
        return pointing;
    }

    // 4. Transform to topocentric East-North-Up (ENU) coordinates
    constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;
    constexpr double RAD_TO_DEG = 180.0 / 3.14159265358979323846;

    double phi = m_position.latitude * DEG_TO_RAD;
    double lambda = m_position.longitude * DEG_TO_RAD;

    double sinPhi = std::sin(phi);
    double cosPhi = std::cos(phi);
    double sinLam = std::sin(lambda);
    double cosLam = std::cos(lambda);

    double east = -sinLam * dx + cosLam * dy;
    double north = -sinPhi * cosLam * dx - sinPhi * sinLam * dy + cosPhi * dz;
    double up = cosPhi * cosLam * dx + cosPhi * sinLam * dy + sinPhi * dz;

    // Azimuth: clockwise from true North [0, 360)
    double az = std::atan2(east, north) * RAD_TO_DEG;
    if (az < 0.0) {
        az += 360.0;
    }
    pointing.azimuthDeg = az;

    // Elevation: angle above local horizon [-90, +90]
    double horizDist = std::sqrt(east * east + north * north);
    double elev = std::atan2(up, horizDist) * RAD_TO_DEG;
    pointing.elevationDeg = elev;

    // Minimum elevation mask threshold
    double minMask = m_feederProfile.minElevationDeg;
    pointing.isVisible = (elev >= minMask);

    // 5. Line-of-sight velocity and Doppler shift
    Utils::Position3D satVel = sat.GetOrbitState().GetEcefVelocity(); // km/s
    double u_x = dx / dist;
    double u_y = dy / dist;
    double u_z = dz / dist;
    double losVelKms = satVel.x * u_x + satVel.y * u_y + satVel.z * u_z;
    pointing.losVelocityKms = losVelKms;

    // Doppler shift at carrier frequency
    constexpr double SPEED_OF_LIGHT = 299792458.0;
    pointing.dopplerShiftHz = -(losVelKms * 1000.0 / SPEED_OF_LIGHT) * m_feederProfile.carrierFrequencyHz;

    return pointing;
}

Signal::SignalInfo Gateway::CalculateFeederSignal(const Orbit::Satellite& sat,
                                                  const Signal::SignalEngine* signalEngine) const {
    auto pointing = CalculateDishPointing(sat);

    if (signalEngine) {
        Coverage::CoverageInfo cov;
        cov.SetDistanceMeters(pointing.slantRangeMeters);
        cov.SetElevationAngleDeg(pointing.elevationDeg);
        cov.SetLosVelocityKms(pointing.losVelocityKms);
        cov.SetVisible(pointing.isVisible);
        auto sig = signalEngine->CalculateSignal(cov, m_feederProfile.carrierFrequencyHz / 1.0e9);
        // Apply gateway high-gain parabolic dish antenna gain (45 dBi Ka-band vs UE default 0 dBi)
        double gainDiff = m_feederProfile.antennaGainDbi - signalEngine->GetChannelConfig().rxAntennaGainDbi;
        if (gainDiff > 0.0) {
            sig.SetRsrpDbm(sig.GetRsrpDbm() + gainDiff);
            sig.SetSinrDb(sig.GetSinrDb() + gainDiff);
        }
        return sig;
    }

    Signal::SignalInfo sig;
    constexpr double SPEED_OF_LIGHT = 299792458.0;
    sig.SetLatencySeconds(pointing.slantRangeMeters / SPEED_OF_LIGHT);
    sig.SetDopplerShiftHz(pointing.dopplerShiftHz);

    constexpr double PI = 3.14159265358979323846;
    double fsplDb = 20.0 * std::log10(std::max(pointing.slantRangeMeters, 1.0))
                  + 20.0 * std::log10(m_feederProfile.carrierFrequencyHz)
                  + 20.0 * std::log10(4.0 * PI / SPEED_OF_LIGHT);
    sig.SetPathLossDb(fsplDb);

    double elevRad = std::max(pointing.elevationDeg, 5.0) * (PI / 180.0);
    double atmLossDb = 0.5 / std::sin(elevRad);
    double eirpDbm = m_feederProfile.txPowerDbm + 25.0; // 25 dBi sat feeder antenna
    double rsrpDbm = eirpDbm - fsplDb - atmLossDb + m_feederProfile.antennaGainDbi - 2.0;
    sig.SetRsrpDbm(rsrpDbm);

    double thermalNoiseDbm = -174.0 + 10.0 * std::log10(m_feederProfile.bandwidthHz) + 5.0;
    sig.SetSinrDb(rsrpDbm - thermalNoiseDbm);

    return sig;
}

std::shared_ptr<Orbit::Satellite> Gateway::SelectBestFeederSatellite(
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const {
    
    std::shared_ptr<Orbit::Satellite> bestSat = nullptr;
    double maxElevation = -90.0;

    for (const auto& sat : satellites) {
        if (!sat) {
            continue;
        }
        auto pointing = CalculateDishPointing(*sat);
        if (pointing.isVisible && pointing.elevationDeg > maxElevation) {
            maxElevation = pointing.elevationDeg;
            bestSat = sat;
        }
    }
    return bestSat;
}

bool Gateway::EvaluateFeederHandover(
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites,
    Routing::RoutingEngine* routingEngine,
    double currentTime,
    Metrics::MetricsEngine* metricsEngine,
    double handoverThresholdDeg) {
    
    std::string currentSatId;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        currentSatId = m_connectedSatId;
    }

    std::shared_ptr<Orbit::Satellite> currentSat = nullptr;
    if (!currentSatId.empty()) {
        for (const auto& sat : satellites) {
            if (sat && sat->GetId() == currentSatId) {
                currentSat = sat;
                break;
            }
        }
    }

    bool needsHandover = false;
    if (!currentSat) {
        needsHandover = true;
    } else {
        auto pointing = CalculateDishPointing(*currentSat);
        // If current satellite drops below threshold or is out of line-of-sight
        if (!pointing.isVisible || pointing.elevationDeg < handoverThresholdDeg) {
            needsHandover = true;
        }
    }

    if (needsHandover) {
        auto bestSat = SelectBestFeederSatellite(satellites);
        if (bestSat && bestSat->GetId() != currentSatId) {
            auto bestPointing = CalculateDishPointing(*bestSat);
            return SwitchFeederSatellite(bestSat->GetId(), bestPointing.slantRangeMeters,
                                         routingEngine, currentTime, metricsEngine);
        } else if (!bestSat && !currentSatId.empty()) {
            // Gap in coverage: disconnect feeder link until rising satellite appears
            return SwitchFeederSatellite("", 0.0, routingEngine, currentTime, metricsEngine);
        }
    }

    return false;
}

bool Gateway::SwitchFeederSatellite(
    const std::string& newSatId,
    double distanceMeters,
    Routing::RoutingEngine* routingEngine,
    double currentTime,
    Metrics::MetricsEngine* metricsEngine) {
    
    std::string oldSatId;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_connectedSatId == newSatId) {
            return false;
        }
        oldSatId = m_connectedSatId;
        m_connectedSatId = newSatId;
        m_hasActiveFeederLink = !newSatId.empty();
    }

    // Step 3: Dynamically update RoutingEngine topology graph
    if (routingEngine) {
        if (!oldSatId.empty()) {
            routingEngine->RemoveLink(m_id, oldSatId, /*bidirectional=*/true);
        }
        if (!newSatId.empty()) {
            routingEngine->AddLink(m_id, newSatId, distanceMeters,
                                   /*isInterPlane=*/false, m_feederProfile.dataRateBps,
                                   /*bidirectional=*/true);
        }
    }

    // Record metrics & handover telemetry
    if (metricsEngine) {
        metricsEngine->RecordHandover(m_id, oldSatId, newSatId, currentTime, true, 0.035);
        metricsEngine->RecordMetric("gateway.feeder_handover_count", 1.0, currentTime);
        if (!newSatId.empty()) {
            metricsEngine->RecordMetric("gateway." + m_id + ".active_feeder_distance_km",
                                        distanceMeters / 1000.0, currentTime);
        }
    }

    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO,
        "[Gateway:" + m_id + "] Feeder Link Handover completed: " +
        (oldSatId.empty() ? "None" : oldSatId) + " -> " +
        (newSatId.empty() ? "None" : newSatId) +
        " (dist=" + std::to_string(distanceMeters / 1000.0) + " km) at t=" +
        std::to_string(currentTime) + "s");

    return true;
}

std::shared_ptr<Packet::Packet> Gateway::InjectUplinkPacket(
    Packet::PacketEngine& packetEngine,
    uint64_t packetId,
    uint32_t sizeBytes,
    double currentTime,
    const std::string& destinationId,
    Core::EventScheduler* scheduler) {
    
    auto packet = packetEngine.GeneratePacket(packetId, sizeBytes, currentTime, m_id, destinationId);
    if (!packet) {
        return nullptr;
    }

    if (scheduler) {
        packetEngine.RoutePacketMultiHop(packet, m_id, currentTime, *scheduler, m_feederProfile.dataRateBps);
    }
    return packet;
}

void Gateway::RecordMetrics(Metrics::MetricsEngine* metricsEngine, double currentTime) const {
    if (!metricsEngine) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    metricsEngine->RecordMetric("gateway." + m_id + ".downlink_throughput_mbps", m_stats.throughputMbps, currentTime);
    metricsEngine->RecordMetric("gateway." + m_id + ".jitter_ms", m_stats.jitterSeconds * 1000.0, currentTime);
    metricsEngine->RecordMetric("gateway." + m_id + ".average_latency_ms", m_stats.averageLatencySeconds * 1000.0, currentTime);
    metricsEngine->RecordMetric("gateway." + m_id + ".feeder_packet_drops", static_cast<double>(m_stats.lostPacketsDetected), currentTime);
    metricsEngine->RecordMetric("gateway." + m_id + ".utilization_percent", GetUtilizationPercentInternal(), currentTime);
    metricsEngine->RecordMetric("gateway." + m_id + ".active_feeder", m_hasActiveFeederLink ? 1.0 : 0.0, currentTime);
}

} // namespace NtnSim::Gateway
