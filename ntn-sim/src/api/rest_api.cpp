#include "ntn-sim/api/rest_api.h"

namespace NtnSim::Api {

RestApi::RestApi() {}

RestApi::~RestApi() {
    StopServer();
}

bool RestApi::StartServer(int port) {
    m_port = port;
    m_isRunning = true;
    // Skeleton placeholder for server startup
    return true;
}

void RestApi::StopServer() {
    m_isRunning = false;
}

std::string RestApi::HandleGetSatellites() const {
    // Skeleton placeholder
    return "[]";
}

std::string RestApi::HandleGetUsers() const {
    // Skeleton placeholder
    return "[]";
}

std::string RestApi::HandleGetMetrics() const {
    // Skeleton placeholder
    return "{}";
}

bool RestApi::HandlePostStart() {
    // Skeleton placeholder
    return true;
}

bool RestApi::HandlePostStop() {
    // Skeleton placeholder
    return true;
}

} // namespace NtnSim::Api
