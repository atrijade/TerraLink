#pragma once

#include "ntn-sim/utils/simulation_config.h"
#include <memory>
#include <string>

namespace NtnSim::Core {
class EventScheduler;
}
namespace NtnSim::Orbit {
class OrbitEngine;
}
namespace NtnSim::User {
class UserEngine;
}
namespace NtnSim::Coverage {
class CoverageEngine;
}
namespace NtnSim::Signal {
class SignalEngine;
}
namespace NtnSim::Handover {
class HandoverEngine;
}
namespace NtnSim::Packet {
class PacketEngine;
}
namespace NtnSim::Routing {
class RoutingEngine;
}
namespace NtnSim::Gateway {
class Gateway;
class GatewayEngine;
}
namespace NtnSim::Metrics {
class MetricsEngine;
}
namespace NtnSim::Api {
class RestApi;
}

namespace NtnSim::Core {

/**
 * @brief Coordinates the setup, execution, and teardown of the simulation run.
 */
class SimulationController {
public:
    SimulationController();
    ~SimulationController();

    /**
     * @brief Loads configuration from file.
     */
    bool LoadConfiguration(const std::string& configFilePath);

    /**
     * @brief Instantiates all engines and binds them together via DI.
     */
    void InitializeEngines();

    /**
     * @brief Launches the discrete-event main loop.
     */
    void StartSimulation();

    /**
     * @brief Explicitly halts the discrete-event main loop.
     */
    void StopSimulation();

    /**
     * @brief Checks if the simulation loop is currently active.
     */
    bool IsSimulationRunning() const;

    // Getters for dependency verification in tests
    EventScheduler* GetScheduler() const;
    Orbit::OrbitEngine* GetOrbitEngine() const;
    User::UserEngine* GetUserEngine() const;
    Coverage::CoverageEngine* GetCoverageEngine() const;
    Signal::SignalEngine* GetSignalEngine() const;
    Handover::HandoverEngine* GetHandoverEngine() const;
    Packet::PacketEngine* GetPacketEngine() const;
    Routing::RoutingEngine* GetRoutingEngine() const;
    Gateway::Gateway* GetGateway() const;
    Gateway::GatewayEngine* GetGatewayEngine() const;
    Metrics::MetricsEngine* GetMetricsEngine() const;
    Api::RestApi* GetRestApi() const;

private:
    std::unique_ptr<EventScheduler> m_scheduler;
    std::unique_ptr<Orbit::OrbitEngine> m_orbitEngine;
    std::unique_ptr<User::UserEngine> m_userEngine;
    std::unique_ptr<Coverage::CoverageEngine> m_coverageEngine;
    std::unique_ptr<Signal::SignalEngine> m_signalEngine;
    std::unique_ptr<Handover::HandoverEngine> m_handoverEngine;
    std::unique_ptr<Packet::PacketEngine> m_packetEngine;
    std::unique_ptr<Routing::RoutingEngine> m_routingEngine;
    std::unique_ptr<Gateway::Gateway> m_gateway;
    std::unique_ptr<Gateway::GatewayEngine> m_gatewayEngine;
    std::unique_ptr<Metrics::MetricsEngine> m_metricsEngine;
    std::unique_ptr<Api::RestApi> m_restApi;

    Utils::SimulationConfig m_config;
    bool m_isRunning{false};
};

} // namespace NtnSim::Core
