#include "ntn-sim/routing/routing_engine.h"

namespace NtnSim::Routing {

RoutingEngine::RoutingEngine() {}

RoutingEngine::~RoutingEngine() {}

std::vector<std::string> RoutingEngine::CalculateRoute(const std::string& srcNodeId, const std::string& destNodeId) {
    (void)srcNodeId;
    (void)destNodeId;
    // Skeleton implementation
    return {srcNodeId, destNodeId};
}

std::string RoutingEngine::ResolveNextHop(const std::string& currentNodeId, const std::string& destNodeId) {
    (void)currentNodeId;
    // Skeleton implementation
    return destNodeId;
}

void RoutingEngine::UpdateTopology() {
    // Skeleton implementation
}

} // namespace NtnSim::Routing
