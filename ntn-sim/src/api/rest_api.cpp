#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/utils/math_utils.h"

#include <sstream>
#include <iomanip>
#include <vector>

namespace NtnSim::Api {

RestApi::RestApi() : m_controller(nullptr) {}

RestApi::RestApi(Core::SimulationController* controller) : m_controller(controller) {}

RestApi::~RestApi() {
    StopServer();
}

void RestApi::SetSimulationController(Core::SimulationController* controller) {
    m_controller = controller;
}

Core::SimulationController* RestApi::GetSimulationController() const {
    return m_controller;
}

Orbit::OrbitEngine* RestApi::GetOrbitEngine() const {
    return m_controller ? m_controller->GetOrbitEngine() : nullptr;
}

Routing::RoutingEngine* RestApi::GetRoutingEngine() const {
    return m_controller ? m_controller->GetRoutingEngine() : nullptr;
}

Gateway::GatewayEngine* RestApi::GetGatewayEngine() const {
    return m_controller ? m_controller->GetGatewayEngine() : nullptr;
}

Gateway::Gateway* RestApi::GetGateway() const {
    return m_controller ? m_controller->GetGateway() : nullptr;
}

Metrics::MetricsEngine* RestApi::GetMetricsEngine() const {
    return m_controller ? m_controller->GetMetricsEngine() : nullptr;
}

User::UserEngine* RestApi::GetUserEngine() const {
    return m_controller ? m_controller->GetUserEngine() : nullptr;
}

Coverage::CoverageEngine* RestApi::GetCoverageEngine() const {
    return m_controller ? m_controller->GetCoverageEngine() : nullptr;
}

Signal::SignalEngine* RestApi::GetSignalEngine() const {
    return m_controller ? m_controller->GetSignalEngine() : nullptr;
}

Packet::PacketEngine* RestApi::GetPacketEngine() const {
    return m_controller ? m_controller->GetPacketEngine() : nullptr;
}

bool RestApi::IsRunning() const {
    return m_isRunning;
}

int RestApi::GetPort() const {
    return m_port;
}

bool RestApi::StartServer(int port) {
    m_port = port;
    m_isRunning = true;
    return true;
}

void RestApi::StopServer() {
    m_isRunning = false;
}

// =========================================================================
// Step 2: REST JSON State Serializers
// =========================================================================

std::string RestApi::HandleGetSatellites() const {
    auto* orbit = GetOrbitEngine();
    if (!orbit) {
        return "[]";
    }

    auto satellites = orbit->GetAllSatellites();
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "[";

    bool first = true;
    for (const auto& sat : satellites) {
        if (!sat) continue;
        if (!first) ss << ",";
        first = false;

        const auto& state = sat->GetOrbitState();
        Utils::Position3D ecefPos = state.GetEcefPosition();
        Utils::Position3D ecefVel = state.GetEcefVelocity();
        Utils::GeodeticPosition geo = Utils::MathUtils::EcefToGeodetic(ecefPos);

        ss << "{"
           << "\"id\":\"" << sat->GetId() << "\","
           << "\"lat\":" << geo.latitude << ","
           << "\"lon\":" << geo.longitude << ","
           << "\"altKm\":" << (geo.altitude / 1000.0) << ","
           << "\"ecef\":{"
           << "\"x\":" << ecefPos.x << ","
           << "\"y\":" << ecefPos.y << ","
           << "\"z\":" << ecefPos.z
           << "},"
           << "\"velKms\":{"
           << "\"x\":" << ecefVel.x << ","
           << "\"y\":" << ecefVel.y << ","
           << "\"z\":" << ecefVel.z
           << "},"
           << "\"altitudeKm\":" << state.GetAltitudeKm()
           << "}";
    }
    ss << "]";
    return ss.str();
}

std::string RestApi::HandleGetTopology() const {
    auto* routing = GetRoutingEngine();
    if (!routing) {
        return "{\"links\":[]}";
    }

    auto allNodes = routing->GetAllNodes();
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\"links\":[";

    bool first = true;
    for (const auto& nodeId : allNodes) {
        auto links = routing->GetLinksForNode(nodeId);
        for (const auto& link : links) {
            if (!first) ss << ",";
            first = false;

            ss << "{"
               << "\"src\":\"" << nodeId << "\","
               << "\"dst\":\"" << link.neighborSatId << "\","
               << "\"distanceKm\":" << (link.distanceMeters / 1000.0) << ","
               << "\"delayMs\":" << (link.propagationDelaySeconds * 1000.0) << ","
               << "\"dataRateGbps\":" << (link.dataRateBps / 1e9) << ","
               << "\"active\":" << (link.isActive ? "true" : "false") << ","
               << "\"isInterPlane\":" << (link.isInterPlane ? "true" : "false")
               << "}";
        }
    }
    ss << "]}";
    return ss.str();
}

std::string RestApi::HandleGetGateways() const {
    std::vector<Gateway::Gateway*> gateways;
    auto* gwEngine = GetGatewayEngine();
    if (gwEngine) {
        auto allGw = gwEngine->GetAllGateways();
        for (auto& g : allGw) {
            if (g) gateways.push_back(g.get());
        }
    }
    if (gateways.empty()) {
        auto* singleGw = GetGateway();
        if (singleGw) {
            gateways.push_back(singleGw);
        }
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "[";

    bool first = true;
    for (auto* gw : gateways) {
        if (!gw) continue;
        if (!first) ss << ",";
        first = false;

        auto pos = gw->GetPosition();
        auto stats = gw->GetStatistics();
        auto profile = gw->GetFeederLinkProfile();

        ss << "{"
           << "\"id\":\"" << gw->GetId() << "\","
           << "\"lat\":" << pos.latitude << ","
           << "\"lon\":" << pos.longitude << ","
           << "\"altM\":" << pos.altitude << ","
           << "\"connectedSat\":\"" << gw->GetConnectedSatelliteId() << "\","
           << "\"hasActiveFeeder\":" << (gw->HasActiveFeederLink() ? "true" : "false") << ","
           << "\"throughputMbps\":" << stats.throughputMbps << ","
           << "\"jitterMs\":" << (stats.jitterSeconds * 1000.0) << ","
           << "\"avgLatencyMs\":" << (stats.averageLatencySeconds * 1000.0) << ","
           << "\"packetsReceived\":" << stats.totalPacketsReceived << ","
           << "\"packetsLost\":" << stats.lostPacketsDetected << ","
           << "\"packetsOutOfOrder\":" << stats.outOfOrderPackets << ","
           << "\"utilizationPercent\":" << gw->GetUtilizationPercent() << ","
           << "\"feederProfile\":{"
           << "\"carrierFreqGhz\":" << (profile.carrierFrequencyHz / 1e9) << ","
           << "\"bandwidthMhz\":" << (profile.bandwidthHz / 1e6) << ","
           << "\"dataRateGbps\":" << (profile.dataRateBps / 1e9) << ","
           << "\"minElevationDeg\":" << profile.minElevationDeg
           << "}"
           << "}";
    }
    ss << "]";
    return ss.str();
}

std::string RestApi::HandleGetUsers() const {
    auto* userEngine = GetUserEngine();
    if (!userEngine) {
        return "[]";
    }

    auto users = userEngine->GetAllUsers();
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "[";

    bool first = true;
    for (const auto& u : users) {
        if (!u) continue;
        if (!first) ss << ",";
        first = false;

        auto pos = u->GetPosition();
        ss << "{"
           << "\"id\":\"" << u->GetId() << "\","
           << "\"lat\":" << pos.latitude << ","
           << "\"lon\":" << pos.longitude << ","
           << "\"altM\":" << pos.altitude << ","
           << "\"servingSat\":\"" << u->GetServingSatelliteId() << "\","
           << "\"trafficRatePps\":" << u->GetTrafficRatePps()
           << "}";
    }
    ss << "]";
    return ss.str();
}

std::string RestApi::HandleGetMetrics() const {
    auto* metrics = GetMetricsEngine();
    double simTime = 0.0;
    if (m_controller && m_controller->GetScheduler()) {
        simTime = m_controller->GetScheduler()->GetCurrentTime();
    }

    if (!metrics) {
        std::ostringstream ss;
        ss << "{\"simTime\":" << simTime << "}";
        return ss.str();
    }

    auto traffic = metrics->CalculateTrafficKPIs();
    auto handovers = metrics->CalculateHandoverKPIs();
    auto radio = metrics->CalculateRadioKPIs();

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "{"
       << "\"simTime\":" << simTime << ","
       << "\"traffic\":{"
       << "\"packetsSent\":" << traffic.packetsSent << ","
       << "\"packetsDelivered\":" << traffic.packetsReceived << ","
       << "\"packetsDropped\":" << traffic.packetsDropped << ","
       << "\"pdrPercent\":" << traffic.packetDeliveryRatioPercent << ","
       << "\"avgLatencyMs\":" << (traffic.meanLatencySeconds * 1000.0) << ","
       << "\"medianLatencyMs\":" << (traffic.p50LatencySeconds * 1000.0) << ","
       << "\"p95LatencyMs\":" << (traffic.p95LatencySeconds * 1000.0) << ","
       << "\"p99LatencyMs\":" << (traffic.p99LatencySeconds * 1000.0) << ","
       << "\"jitterMs\":" << (traffic.jitterSeconds * 1000.0) << ","
       << "\"throughputMbps\":" << traffic.throughputMbps << ","
       << "\"dropsQueueOverflow\":" << traffic.dropsQueueOverflow << ","
       << "\"dropsTtlExpired\":" << traffic.dropsTtlExpired << ","
       << "\"dropsRadioLoss\":" << traffic.dropsRadioLoss << ","
       << "\"dropsNoRoute\":" << traffic.dropsNoRoute
       << "},"
       << "\"handovers\":{"
       << "\"totalAttempts\":" << handovers.totalAttempts << ","
       << "\"totalSuccesses\":" << handovers.totalSuccesses << ","
       << "\"totalFailures\":" << handovers.totalFailures << ","
       << "\"successRatePercent\":" << handovers.successRatePercent << ","
       << "\"pingPongCount\":" << handovers.pingPongCount << ","
       << "\"pingPongRatePercent\":" << handovers.pingPongRatePercent << ","
       << "\"avgInterruptionMs\":" << (handovers.averageInterruptionTimeSeconds * 1000.0)
       << "},"
       << "\"radio\":{"
       << "\"serviceAvailabilityPercent\":" << radio.serviceAvailabilityPercent << ","
       << "\"outageProbabilityPercent\":" << radio.outageProbabilityPercent << ","
       << "\"meanRsrpDbm\":" << radio.meanRsrpDbm << ","
       << "\"meanSinrDb\":" << radio.meanSinrDb << ","
       << "\"meanDopplerHz\":" << radio.meanDopplerShiftHz << ","
       << "\"maxDopplerHz\":" << radio.maxDopplerShiftHz
       << "}"
       << "}";
    return ss.str();
}

bool RestApi::HandlePostStart() {
    if (m_controller) {
        return true;
    }
    return false;
}

bool RestApi::HandlePostStop() {
    if (m_controller) {
        m_controller->StopSimulation();
        return true;
    }
    return false;
}

} // namespace NtnSim::Api
