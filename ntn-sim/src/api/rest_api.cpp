#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/packet/packet_engine.h"

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

std::string RestApi::HandleGetSatellites() const {
    return "[]";
}

std::string RestApi::HandleGetUsers() const {
    return "[]";
}

std::string RestApi::HandleGetMetrics() const {
    return "{}";
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
