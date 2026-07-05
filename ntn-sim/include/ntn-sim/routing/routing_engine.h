#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace NtnSim::Routing {

/**
 * @brief Computes routing pathways over the dynamic satellite constellation topology.
 */
class RoutingEngine {
public:
    RoutingEngine();
    ~RoutingEngine();

    /**
     * @brief Computes a complete hop-by-hop route between source and destination.
     * @param srcNodeId Origin identifier.
     * @param destNodeId Target identifier.
     * @return Ordered list of node IDs.
     */
    std::vector<std::string> CalculateRoute(const std::string& srcNodeId, const std::string& destNodeId);

    /**
     * @brief Resolves the immediate next hop for a packet.
     */
    std::string ResolveNextHop(const std::string& currentNodeId, const std::string& destNodeId);

    /**
     * @brief Updates internal connectivity topology tables.
     */
    void UpdateTopology();

private:
    std::unordered_map<std::string, std::vector<std::string>> m_topologyMap;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Routing
