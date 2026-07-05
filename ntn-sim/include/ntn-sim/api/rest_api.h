#pragma once

#include <string>

namespace NtnSim::Api {

/**
 * @brief Placeholder for future REST API server to monitor and control simulation.
 */
class RestApi {
public:
    RestApi();
    ~RestApi();

    /**
     * @brief Starts the REST API listening server on a specified port.
     */
    bool StartServer(int port);

    /**
     * @brief Stops the REST API listening server.
     */
    void StopServer();

    /**
     * @brief REST Placeholder GET /satellites
     */
    std::string HandleGetSatellites() const;

    /**
     * @brief REST Placeholder GET /users
     */
    std::string HandleGetUsers() const;

    /**
     * @brief REST Placeholder GET /metrics
     */
    std::string HandleGetMetrics() const;

    /**
     * @brief REST Placeholder POST /start
     */
    bool HandlePostStart();

    /**
     * @brief REST Placeholder POST /stop
     */
    bool HandlePostStop();

private:
    bool m_isRunning{false};
    int m_port{8080};
};

} // namespace NtnSim::Api
