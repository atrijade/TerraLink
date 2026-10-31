#pragma once

#include <string>
#include <memory>
#include <atomic>

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
 *        Embeds a lightweight cpp-httplib HTTP server running in a worker thread.
 */
class RestApi {
public:
    RestApi();
    explicit RestApi(Core::SimulationController* controller);
    ~RestApi();

    // Non-copyable (due to server thread and unique_ptr)
    RestApi(const RestApi&) = delete;
    RestApi& operator=(const RestApi&) = delete;

    // Movable
    RestApi(RestApi&&) noexcept;
    RestApi& operator=(RestApi&&) noexcept;

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
     * @brief Starts the REST API listening server on a specified port (default 8080).
     *        Binds HTTP routes to JSON handler methods and listens on 0.0.0.0 in a worker thread.
     */
    bool StartServer(int port = 8080);

    /**
     * @brief Stops the REST API listening server and joins the worker thread.
     */
    void StopServer();

    bool IsRunning() const;
    int GetPort() const;

    // =========================================================================
    // REST JSON State Serializers
    // =========================================================================

    /**
     * @brief REST Handler GET /satellites
     *        Serializes satellite IDs, ECEF positions, velocities, and geodetic coordinates.
     */
    std::string HandleGetSatellites() const;

    /**
     * @brief REST Handler GET /topology
     *        Serializes active ISL cross-links and feeder links.
     */
    std::string HandleGetTopology() const;

    /**
     * @brief REST Handler GET /gateways
     *        Serializes gateway stats (throughput, jitter, connected satellite, loss).
     */
    std::string HandleGetGateways() const;

    /**
     * @brief REST Handler GET /users
     *        Serializes user equipment terminals, coordinates, and serving satellites.
     */
    std::string HandleGetUsers() const;

    /**
     * @brief REST Handler GET /metrics
     *        Serializes aggregate 3GPP KPIs (PDR, latency, percentiles, handovers, drops).
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
    std::atomic<bool> m_isRunning{false};
    int m_port{8080};

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace NtnSim::Api
