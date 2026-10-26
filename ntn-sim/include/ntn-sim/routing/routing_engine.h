#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>

namespace NtnSim::Orbit {
    class Satellite;
}

namespace NtnSim::Routing {

/**
 * @brief Routing metric used by Dijkstra's shortest-path algorithm to weight graph edges.
 */
enum class RoutingMetric {
    MinLatency,      ///< Physical metric: minimizes propagation delay (T_prop = d / c)
    MinHops,         ///< Topological metric: minimizes hop count (uniform weight = 1.0)
    CongestionAware  ///< Delay metric: minimizes propagation delay + node queuing delay (T_prop + T_queue)
};

/**
 * @brief Represents an optical Inter-Satellite Link (ISL) or feeder link connecting two nodes.
 */
struct IslLink {
    std::string neighborSatId;           ///< Adjacent node or satellite identifier
    double distanceMeters{0.0};          ///< Physical Euclidean distance (meters)
    double propagationDelaySeconds{0.0}; ///< Propagation delay (d / c) in seconds
    double dataRateBps{10.0e9};          ///< Optical cross-link data rate (default: 10 Gbps)
    bool isActive{true};                 ///< True if operational, false if disabled (e.g. polar shutoff or failure)
    bool isInterPlane{false};            ///< True if cross-plane link, false if intra-plane or feeder
};

/**
 * @brief Represents a node's local active topology and adjacent cross-links.
 */
struct NodeTopology {
    std::string nodeId;                  ///< Current node identifier
    std::vector<IslLink> activeLinks;    ///< Operational links originating from this node
};

/**
 * @brief Computes routing pathways over the dynamic satellite constellation topology.
 *
 * Implements:
 * - Phase 1: Constellation Topology & ISL Mesh Modeling
 *   * 4-ISL mesh architecture per satellite (2 intra-plane front/back, 2 inter-plane left/right)
 *   * Dynamic inter-plane distance and propagation delay modeling
 *   * Polar latitude link shut-off (lat > 70 deg) to prevent optical gimbal limit violations
 * - Phase 2: Shortest-Path Routing Algorithms (Dijkstra)
 *   * Configurable cost metrics: MinLatency, MinHops, CongestionAware
 *   * End-to-end route calculation (CalculateRoute)
 *   * O(1) forwarding table cache and hop-by-hop lookup (ResolveNextHop)
 */
class RoutingEngine {
public:
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0; ///< Speed of light in vacuum (m/s)
    static constexpr double DEFAULT_DATA_RATE_BPS = 10.0e9;   ///< Default laser link data rate (10 Gbps)
    static constexpr double DEFAULT_POLAR_LAT_DEG = 70.0;     ///< Default polar shut-off latitude threshold (deg)

    RoutingEngine();
    ~RoutingEngine();

    // -----------------------------------------------------------------------
    // Configuration & Metric Control
    // -----------------------------------------------------------------------

    /**
     * @brief Configures the cost metric evaluated during path-finding.
     */
    void SetMetricType(RoutingMetric metric);

    /**
     * @brief Returns current routing cost metric.
     */
    RoutingMetric GetMetricType() const;

    /**
     * @brief Sets the polar latitude threshold above which inter-plane ISLs are shut down.
     * @param maxLatDeg Latitude in degrees (e.g. 70.0).
     */
    void SetMaxPolarLatitudeDeg(double maxLatDeg);

    /**
     * @brief Returns the current polar shut-off latitude threshold in degrees.
     */
    double GetMaxPolarLatitudeDeg() const;

    // -----------------------------------------------------------------------
    // Phase 1: Constellation Topology & ISL Mesh Modeling
    // -----------------------------------------------------------------------

    /**
     * @brief Builds a 4-ISL mesh topology for a Walker Delta constellation.
     *        Establishes:
     *        - 2 Intra-Plane ISLs (front and back in same orbital ring)
     *        - 2 Inter-Plane ISLs (left and right to adjacent orbital planes)
     *        Inter-plane links are dynamically checked against polar latitude limits.
     * @param satellites Vector of satellite shared pointers.
     * @param planes Number of orbital planes (P).
     * @param satsPerPlane Satellites per orbital plane (S).
     * @param maxPolarLatDeg Latitude threshold where inter-plane links are deactivated (deg).
     */
    void BuildConstellationTopology(
        const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites,
        int planes,
        int satsPerPlane,
        double maxPolarLatDeg = DEFAULT_POLAR_LAT_DEG);

    /**
     * @brief Updates internal connectivity topology tables.
     *        Recalculates ISL distances, optical propagation delays, and polar link shut-offs
     *        based on current satellite ECEF coordinates.
     */
    void UpdateTopology();

    // -----------------------------------------------------------------------
    // Ad-hoc Link & Graph Management (Gateways, UEs, custom links)
    // -----------------------------------------------------------------------

    /**
     * @brief Adds or updates a directional or bidirectional link between two nodes.
     */
    void AddLink(const std::string& srcNodeId, const std::string& destNodeId,
                 double distanceMeters, bool isInterPlane = false,
                 double dataRateBps = DEFAULT_DATA_RATE_BPS, bool bidirectional = true);

    /**
     * @brief Removes a link between two nodes.
     */
    void RemoveLink(const std::string& srcNodeId, const std::string& destNodeId,
                    bool bidirectional = true);

    /**
     * @brief Sets the active operational status of an existing link.
     */
    void SetLinkActive(const std::string& srcNodeId, const std::string& destNodeId,
                       bool active, bool bidirectional = true);

    /**
     * @brief Checks if a directed link exists from srcNodeId to destNodeId.
     */
    bool HasLink(const std::string& srcNodeId, const std::string& destNodeId) const;

    /**
     * @brief Retrieves link details between two nodes if present.
     */
    std::optional<IslLink> GetLink(const std::string& srcNodeId, const std::string& destNodeId) const;

    // -----------------------------------------------------------------------
    // Topology Inspection
    // -----------------------------------------------------------------------

    /**
     * @brief Returns a list of all unique node identifiers registered in the graph.
     */
    std::vector<std::string> GetAllNodes() const;

    /**
     * @brief Returns all links originating from the specified node.
     */
    std::vector<IslLink> GetLinksForNode(const std::string& nodeId) const;

    /**
     * @brief Returns the local active topology (only active links) for a node.
     */
    NodeTopology GetNodeTopology(const std::string& nodeId) const;

    /**
     * @brief Returns the total count of operational links in the entire graph.
     */
    size_t GetTotalActiveLinksCount() const;

    // -----------------------------------------------------------------------
    // Congestion Delay Management (For CongestionAware Metric)
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the simulated queuing delay at a specific satellite or node.
     * @param nodeId Node identifier.
     * @param queueDelaySeconds Queuing delay in seconds.
     */
    void SetNodeQueuingDelay(const std::string& nodeId, double queueDelaySeconds);

    /**
     * @brief Returns current queuing delay at a specific node.
     */
    double GetNodeQueuingDelay(const std::string& nodeId) const;

    /**
     * @brief Clears all node queuing delays back to 0.0.
     */
    void ClearQueuingDelays();

    // -----------------------------------------------------------------------
    // Phase 2: Shortest-Path Routing Algorithms (Dijkstra)
    // -----------------------------------------------------------------------

    /**
     * @brief Computes a complete hop-by-hop route between source and destination using Dijkstra.
     * @param srcNodeId Origin identifier (e.g. "ue-1" or "sat-p0-s0").
     * @param destNodeId Target identifier (e.g. "gw-0" or "sat-p1-s2").
     * @return Ordered list of node IDs: ["src", "sat-1", ..., "dest"], or empty if unreachable.
     */
    std::vector<std::string> CalculateRoute(const std::string& srcNodeId, const std::string& destNodeId);

    /**
     * @brief Resolves the immediate next hop for a packet. Cached for O(1) forwarding lookup.
     * @param currentNodeId Node where packet is currently located.
     * @param destNodeId Ultimate destination node.
     * @return Next node ID to forward packet to, or empty string if unreachable.
     */
    std::string ResolveNextHop(const std::string& currentNodeId, const std::string& destNodeId);

    // -----------------------------------------------------------------------
    // Route Analysis Helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Computes total physical Euclidean distance across an ordered route.
     */
    double CalculateRouteDistance(const std::vector<std::string>& path) const;

    /**
     * @brief Computes total optical propagation delay across an ordered route.
     */
    double CalculateRouteLatency(const std::vector<std::string>& path) const;

    /**
     * @brief Computes total cost across an ordered route under current RoutingMetric.
     */
    double CalculateRouteCost(const std::vector<std::string>& path) const;

    /**
     * @brief Clears all topology, registered satellites, and cached forwarding tables.
     */
    void ClearTopology();

private:
    // Internal helper methods
    double CalculateLatitudeDeg(const Utils::Position3D& ecef) const;
    double DoGetEdgeWeight(const std::string& fromNode, const IslLink& link) const;
    std::vector<std::string> DoCalculateRoute(const std::string& srcNodeId, const std::string& destNodeId);
    void InvalidateForwardingCache();

    RoutingMetric m_metric{RoutingMetric::MinLatency};
    double m_maxPolarLatDeg{DEFAULT_POLAR_LAT_DEG};

    // Adjacency list: nodeId -> list of outgoing links
    std::unordered_map<std::string, std::vector<IslLink>> m_adjacencyList;

    // Node queuing delays for CongestionAware routing (nodeId -> seconds)
    std::unordered_map<std::string, double> m_nodeQueuingDelays;

    // Forwarding table cache for O(1) lookup: [srcNodeId][destNodeId] -> nextHopId
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_forwardingTable;

    // Tracked satellites for dynamic topology updates
    std::vector<std::shared_ptr<Orbit::Satellite>> m_satellites;
    std::unordered_map<std::string, std::shared_ptr<Orbit::Satellite>> m_satelliteMap;
    int m_planes{0};
    int m_satsPerPlane{0};

    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Routing
