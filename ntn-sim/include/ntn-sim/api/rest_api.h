#pragma once

#include <string>

namespace NtnSim::Core {
class SimulationController;
}

namespace NtnSim::Orbit {
class OrbitEngine;
}

namespace NtnSim::Routing {
class RoutingEngine;
}

namespace NtnSim::Gateway {
class GatewayEngine;
class Gateway;
}

namespace NtnSim::Metrics {
class MetricsEngine;
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

namespace NtnSim::Packet {
class PacketEngine;
}

namespace NtnSim::Api {

/**
 * @brief REST API server component to monitor and control the simulation run.
 *        Wired with the central SimulationController to access all core engines.
 */
class RestApi {
public:
    RestApi();
    explicit RestApi(Core::SimulationController* controller);
    ~RestApi();

    /**
     * @brief Injects the central SimulationController instance.
     */
    void SetSimulationController(Core::SimulationController* controller);
    Core::SimulationController* GetSimulationController() const;

    // Engine Accessors via Controller
    Orbit::OrbitEngine* GetOrbitEngine() const;
    Routing::RoutingEngine* GetRoutingEngine() const;
    Gateway::GatewayEngine* GetGatewayEngine() const;
    Gateway::Gateway* GetGateway() const;
    Metrics::MetricsEngine* GetMetricsEngine() const;
    User::UserEngine* GetUserEngine() const;
    Coverage::CoverageEngine* GetCoverageEngine() const;
    Signal::SignalEngine* GetSignalEngine() const;
    Packet::PacketEngine* GetPacketEngine() const;

    /**
     * @brief Starts the REST API listening server on a specified port.
     */
    bool StartServer(int port);

    /**
     * @brief Stops the REST API listening server.
     */
    void StopServer();

    bool IsRunning() const;
    int GetPort() const;

    /**
     * @brief REST Handler GET /satellites
     */
    std::string HandleGetSatellites() const;

    /**
     * @brief REST Handler GET /users
     */
    std::string HandleGetUsers() const;

    /**
     * @brief REST Handler GET /metrics
     */
    std::string HandleGetMetrics() const;

    /**
     * @brief REST Handler POST /start
     */
    bool HandlePostStart();

    /**
     * @brief REST Handler POST /stop
     */
    bool HandlePostStop();

private:
    Core::SimulationController* m_controller{nullptr};
    bool m_isRunning{false};
    int m_port{8080};
};

} // namespace NtnSim::Api
