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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "httplib.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>

namespace NtnSim::Api {

struct RestApi::Impl {
    httplib::Server server;
    std::thread workerThread;
};

RestApi::RestApi()
    : m_controller(nullptr), m_isRunning(false), m_port(8080), m_impl(std::make_unique<Impl>()) {}

RestApi::RestApi(Core::SimulationController* controller)
    : m_controller(controller), m_isRunning(false), m_port(8080), m_impl(std::make_unique<Impl>()) {}

RestApi::~RestApi() {
    StopServer();
}

RestApi::RestApi(RestApi&& other) noexcept
    : m_controller(other.m_controller),
      m_isRunning(other.m_isRunning.load()),
      m_port(other.m_port),
      m_impl(std::move(other.m_impl)) {
    other.m_controller = nullptr;
    other.m_isRunning.store(false);
}

RestApi& RestApi::operator=(RestApi&& other) noexcept {
    if (this != &other) {
        StopServer();
        m_controller = other.m_controller;
        m_isRunning.store(other.m_isRunning.load());
        m_port = other.m_port;
        m_impl = std::move(other.m_impl);
        other.m_controller = nullptr;
        other.m_isRunning.store(false);
    }
    return *this;
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
    return m_isRunning.load();
}

int RestApi::GetPort() const {
    return m_port;
}

bool RestApi::StartServer(int port) {
    if (m_isRunning.load()) {
        StopServer();
    }

    m_port = port;
    m_impl = std::make_unique<Impl>();

    // 1. CORS headers configuration for web dashboards and visualization clients
    m_impl->server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With"}
    });

    m_impl->server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
        res.set_content("", "text/plain");
    });

    // 2. Bind GET telemetry and state routes (support both root path and /api/ prefix)
    auto bindGetHandler = [this](const std::string& path, auto handlerFunc) {
        auto cb = [this, handlerFunc](const httplib::Request&, httplib::Response& res) {
            res.set_content((this->*handlerFunc)(), "application/json");
        };
        m_impl->server.Get(path, cb);
        m_impl->server.Get("/api" + path, cb);
    };

    bindGetHandler("/satellites", &RestApi::HandleGetSatellites);
    bindGetHandler("/topology", &RestApi::HandleGetTopology);
    bindGetHandler("/gateways", &RestApi::HandleGetGateways);
    bindGetHandler("/users", &RestApi::HandleGetUsers);
    bindGetHandler("/metrics", &RestApi::HandleGetMetrics);

    // Health and server status routes
    auto statusCb = [this](const httplib::Request&, httplib::Response& res) {
        std::ostringstream ss;
        ss << "{\n"
           << "  \"status\": \"RUNNING\",\n"
           << "  \"port\": " << m_port << ",\n"
           << "  \"controllerConnected\": " << (m_controller ? "true" : "false") << "\n"
           << "}";
        res.set_content(ss.str(), "application/json");
    };
    m_impl->server.Get("/health", statusCb);
    m_impl->server.Get("/api/health", statusCb);
    m_impl->server.Get("/status", statusCb);
    m_impl->server.Get("/api/status", statusCb);

    // 3. Bind POST control routes
    auto startCb = [this](const httplib::Request&, httplib::Response& res) {
        bool ok = HandlePostStart();
        std::ostringstream ss;
        ss << "{\"success\": " << (ok ? "true" : "false") << ", \"status\": \"STARTED\"}";
        res.set_content(ss.str(), "application/json");
    };
    m_impl->server.Post("/start", startCb);
    m_impl->server.Post("/api/start", startCb);

    auto stopCb = [this](const httplib::Request&, httplib::Response& res) {
        bool ok = HandlePostStop();
        std::ostringstream ss;
        ss << "{\"success\": " << (ok ? "true" : "false") << ", \"status\": \"STOPPED\"}";
        res.set_content(ss.str(), "application/json");
    };
    m_impl->server.Post("/stop", stopCb);
    m_impl->server.Post("/api/stop", stopCb);

    // 4. Start background worker thread listening on 0.0.0.0
    m_impl->workerThread = std::thread([this, port]() {
        m_impl->server.listen("0.0.0.0", port);
    });

    // Wait until server is listening or timeout (up to 1500ms)
    auto startTime = std::chrono::steady_clock::now();
    while (!m_impl->server.is_running()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count() > 1500) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    m_isRunning.store(m_impl->server.is_running());
    return m_isRunning.load();
}

void RestApi::StopServer() {
    if (m_impl) {
        if (m_impl->server.is_running()) {
            m_impl->server.stop();
        }
        if (m_impl->workerThread.joinable()) {
            m_impl->workerThread.join();
        }
    }
    m_isRunning.store(false);
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
