#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/handover/handover_engine.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/logger/logger.h"

namespace NtnSim::Core {

SimulationController::SimulationController() {}

SimulationController::~SimulationController() {}

bool SimulationController::LoadConfiguration(const std::string& configFilePath) {
    (void)configFilePath;
    // Skeleton implementation
    return true;
}

void SimulationController::InitializeEngines() {
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO, "Initializing simulation engines...");

    m_scheduler = std::make_unique<EventScheduler>();
    m_orbitEngine = std::make_unique<Orbit::OrbitEngine>();
    m_userEngine = std::make_unique<User::UserEngine>();
    m_coverageEngine = std::make_unique<Coverage::CoverageEngine>();
    m_signalEngine = std::make_unique<Signal::SignalEngine>();
    m_handoverEngine = std::make_unique<Handover::HandoverEngine>();
    m_packetEngine = std::make_unique<Packet::PacketEngine>();
    m_routingEngine = std::make_unique<Routing::RoutingEngine>();
    
    Utils::GeodeticPosition gwPos{37.7749, -122.4194, 10.0};
    m_gateway = std::make_unique<Gateway::Gateway>("gw-0", gwPos);
    m_metricsEngine = std::make_unique<Metrics::MetricsEngine>();
    m_restApi = std::make_unique<Api::RestApi>(this);

    m_gatewayEngine = std::make_unique<Gateway::GatewayEngine>(m_metricsEngine.get(), m_routingEngine.get());
    m_gatewayEngine->SetOrbitEngine(m_orbitEngine.get());
    m_gatewayEngine->SetCoverageEngine(m_coverageEngine.get());
    m_gatewayEngine->SetSignalEngine(m_signalEngine.get());
    m_gatewayEngine->SetPacketEngine(m_packetEngine.get());
    m_gatewayEngine->AddGateway(m_gateway.get());

    // Wire telemetry, routing, and gateway into engines
    m_packetEngine->SetMetricsEngine(m_metricsEngine.get());
    m_handoverEngine->SetMetricsEngine(m_metricsEngine.get());
    m_packetEngine->SetRoutingEngine(m_routingEngine.get());
    m_packetEngine->RegisterGateway(m_gateway.get());
}

void SimulationController::StartSimulation() {
    if (!m_scheduler) {
        InitializeEngines();
    }
    
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO, "Starting discrete-event simulation loop...");
    m_isRunning = true;

    // Schedule initial event and run loop
    m_scheduler->ScheduleEvent(0.0, 10, EventType::SimulationStart, []() {
        Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO, "SimulationStart event fired.");
    });

    m_scheduler->ScheduleEvent(m_config.GetDurationSeconds(), 10, EventType::SimulationStop, [this]() {
        Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO, "SimulationStop event fired.");
        StopSimulation();
    });

    while (m_isRunning && !m_scheduler->IsEmpty()) {
        m_scheduler->ProcessNextEvent();
    }
}

void SimulationController::StopSimulation() {
    Logger::Logger::GetInstance().Log(Logger::LogLevel::INFO, "Stopping simulation loop.");
    m_isRunning = false;
}

bool SimulationController::IsSimulationRunning() const {
    return m_isRunning;

}

EventScheduler* SimulationController::GetScheduler() const { return m_scheduler.get(); }
Orbit::OrbitEngine* SimulationController::GetOrbitEngine() const { return m_orbitEngine.get(); }
User::UserEngine* SimulationController::GetUserEngine() const { return m_userEngine.get(); }
Coverage::CoverageEngine* SimulationController::GetCoverageEngine() const { return m_coverageEngine.get(); }
Signal::SignalEngine* SimulationController::GetSignalEngine() const { return m_signalEngine.get(); }
Handover::HandoverEngine* SimulationController::GetHandoverEngine() const { return m_handoverEngine.get(); }
Packet::PacketEngine* SimulationController::GetPacketEngine() const { return m_packetEngine.get(); }
Routing::RoutingEngine* SimulationController::GetRoutingEngine() const { return m_routingEngine.get(); }
Gateway::Gateway* SimulationController::GetGateway() const { return m_gateway.get(); }
Gateway::GatewayEngine* SimulationController::GetGatewayEngine() const { return m_gatewayEngine.get(); }
Metrics::MetricsEngine* SimulationController::GetMetricsEngine() const { return m_metricsEngine.get(); }
Api::RestApi* SimulationController::GetRestApi() const { return m_restApi.get(); }

} // namespace NtnSim::Core
