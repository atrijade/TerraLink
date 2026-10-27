#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/utils/math_utils.h"

#include <queue>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace NtnSim::Routing {

RoutingEngine::RoutingEngine() {}

RoutingEngine::~RoutingEngine() {}

// ===========================================================================
// Configuration & Metric Control
// ===========================================================================

void RoutingEngine::SetMetricType(RoutingMetric metric) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_metric != metric) {
        m_metric = metric;
        InvalidateForwardingCache();
    }
}

RoutingMetric RoutingEngine::GetMetricType() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metric;
}

void RoutingEngine::SetMaxPolarLatitudeDeg(double maxLatDeg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxPolarLatDeg = maxLatDeg;
    InvalidateForwardingCache();
}

double RoutingEngine::GetMaxPolarLatitudeDeg() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxPolarLatDeg;
}

// ===========================================================================
// Helper: Latitude Calculation from ECEF Coordinates
// ===========================================================================

double RoutingEngine::CalculateLatitudeDeg(const Utils::Position3D& ecef) const {
    // Euclidean magnitude from Earth center
    double r = std::sqrt(ecef.x * ecef.x + ecef.y * ecef.y + ecef.z * ecef.z);
    if (r < 1.0) {
        return 0.0;
    }
    // sin(latitude) = z / r in Earth-Centered, Earth-Fixed (ECEF) frame
    double sinLat = std::clamp(ecef.z / r, -1.0, 1.0);
    return std::asin(sinLat) * 180.0 / std::numbers::pi;
}

// ===========================================================================
// Phase 1: Constellation Topology & ISL Mesh Modeling
// ===========================================================================

/**
 * In a Walker Delta constellation, satellites form an orbital grid (Manhattan-street mesh).
 * Each satellite establishes 4 optical cross-links (ISLs):
 *
 *                  Plane p-1        Plane p        Plane p+1
 *                      │               │               │
 *  Sat s+1 ─────────── o ───────────── o ───────────── o
 *                      │               │ (Intra-plane) │
 *                      │       (Inter) │               │
 *  Sat s   ─────────── o ──────────── [SAT] ────────── o ─── (Inter-plane)
 *                      │       -plane  │               │
 *                      │               │ (Intra-plane) │
 *  Sat s-1 ─────────── o ───────────── o ───────────── o
 *                      │               │               │
 *
 * 1. Intra-Plane ISLs (Fixed / Always Connected):
 *    - Front: (p, (s + 1) % S)
 *    - Back:  (p, (s - 1 + S) % S)
 *    Distance is constant along the orbital ring (typically 2000 - 3000 km).
 *
 * 2. Inter-Plane ISLs (Dynamic / Left & Right):
 *    - Right: ((p + 1) % P, s)
 *    - Left:  ((p - 1 + P) % P, s)
 *    Distance varies dynamically as planes converge near the poles.
 *    Gimbal tracking limits require links to temporarily shut off when |lat| > 70 deg.
 */
void RoutingEngine::BuildConstellationTopology(
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites,
    int planes,
    int satsPerPlane,
    double maxPolarLatDeg) {

    std::lock_guard<std::mutex> lock(m_mutex);

    m_adjacencyList.clear();
    m_forwardingTable.clear();
    m_satellites = satellites;
    m_satelliteMap.clear();
    m_planes = planes;
    m_satsPerPlane = satsPerPlane;
    m_maxPolarLatDeg = maxPolarLatDeg;

    if (planes <= 0 || satsPerPlane <= 0 || satellites.empty()) {
        return;
    }

    // Index satellites by ID for O(1) lookup
    for (const auto& sat : satellites) {
        if (sat) {
            m_satelliteMap[sat->GetId()] = sat;
        }
    }

    // Helper lambda to construct satellite name according to convention "sat-p{p}-s{s}"
    auto GetSatId = [](int p, int s) -> std::string {
        return "sat-p" + std::to_string(p) + "-s" + std::to_string(s);
    };

    // Iterate through every orbital plane and slot
    for (int p = 0; p < planes; ++p) {
        for (int s = 0; s < satsPerPlane; ++s) {
            std::string currentId = GetSatId(p, s);
            auto currentSatIt = m_satelliteMap.find(currentId);
            if (currentSatIt == m_satelliteMap.end() || !currentSatIt->second) {
                continue;
            }

            auto currentSat = currentSatIt->second;
            Utils::Position3D currentPos = currentSat->GetOrbitState().GetEcefPosition();
            double currentLat = CalculateLatitudeDeg(currentPos);
            bool isCurrentPolar = (std::abs(currentLat) > m_maxPolarLatDeg);

            // ---------------------------------------------------------------
            // 1. Intra-Plane ISLs (Front and Back in same orbital ring)
            // ---------------------------------------------------------------
            int frontSlot = (s + 1) % satsPerPlane;
            int backSlot = (s - 1 + satsPerPlane) % satsPerPlane;

            std::string frontId = GetSatId(p, frontSlot);
            std::string backId = GetSatId(p, backSlot);

            for (const auto& neighborId : {frontId, backId}) {
                auto nbrIt = m_satelliteMap.find(neighborId);
                double dist = 0.0;
                if (nbrIt != m_satelliteMap.end() && nbrIt->second) {
                    Utils::Position3D nbrPos = nbrIt->second->GetOrbitState().GetEcefPosition();
                    dist = Utils::MathUtils::CalculateDistance(currentPos, nbrPos);
                }

                // If satellites have not been propagated yet (at origin), estimate nominal orbital chord
                if (dist < 1.0) {
                    double r = 6378137.0 + (currentSat->GetOrbitState().GetAltitudeKm() * 1000.0);
                    double dTheta = 2.0 * std::numbers::pi / static_cast<double>(satsPerPlane);
                    dist = 2.0 * r * std::sin(dTheta / 2.0);
                }

                double propDelay = dist / SPEED_OF_LIGHT_M_S;

                IslLink link;
                link.neighborSatId = neighborId;
                link.distanceMeters = dist;
                link.propagationDelaySeconds = propDelay;
                link.dataRateBps = DEFAULT_DATA_RATE_BPS;
                link.isActive = true; // Intra-plane links always stay active over poles
                link.isInterPlane = false;

                m_adjacencyList[currentId].push_back(link);
            }

            // ---------------------------------------------------------------
            // 2. Inter-Plane ISLs (Left and Right to adjacent orbital rings)
            // ---------------------------------------------------------------
            int rightPlane = (p + 1) % planes;
            int leftPlane = (p - 1 + planes) % planes;

            std::string rightId = GetSatId(rightPlane, s);
            std::string leftId = GetSatId(leftPlane, s);

            for (const auto& neighborId : {rightId, leftId}) {
                auto nbrIt = m_satelliteMap.find(neighborId);
                double dist = 0.0;
                bool isNeighborPolar = false;

                if (nbrIt != m_satelliteMap.end() && nbrIt->second) {
                    Utils::Position3D nbrPos = nbrIt->second->GetOrbitState().GetEcefPosition();
                    dist = Utils::MathUtils::CalculateDistance(currentPos, nbrPos);
                    double nbrLat = CalculateLatitudeDeg(nbrPos);
                    isNeighborPolar = (std::abs(nbrLat) > m_maxPolarLatDeg);
                }

                if (dist < 1.0) {
                    // Nominal cross-plane distance estimate at equator
                    double r = 6378137.0 + (currentSat->GetOrbitState().GetAltitudeKm() * 1000.0);
                    double dRaan = 2.0 * std::numbers::pi / static_cast<double>(planes);
                    dist = 2.0 * r * std::sin(dRaan / 2.0);
                }

                double propDelay = dist / SPEED_OF_LIGHT_M_S;

                IslLink link;
                link.neighborSatId = neighborId;
                link.distanceMeters = dist;
                link.propagationDelaySeconds = propDelay;
                link.dataRateBps = DEFAULT_DATA_RATE_BPS;
                // Polar shut-off: Deactivate if either satellite is beyond max polar latitude
                link.isActive = (!isCurrentPolar && !isNeighborPolar);
                link.isInterPlane = true;

                m_adjacencyList[currentId].push_back(link);
            }
        }
    }
}

void RoutingEngine::UpdateTopology() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Invalidate cached forwarding tables since distances and link states will change
    InvalidateForwardingCache();

    if (m_satelliteMap.empty()) {
        return;
    }

    // Recompute distances, optical delays, and polar shut-offs for all links
    for (auto& [nodeId, links] : m_adjacencyList) {
        auto srcIt = m_satelliteMap.find(nodeId);
        if (srcIt == m_satelliteMap.end() || !srcIt->second) {
            continue; // Not a satellite (e.g. gateway or UE)
        }

        Utils::Position3D srcPos = srcIt->second->GetOrbitState().GetEcefPosition();
        double srcLat = CalculateLatitudeDeg(srcPos);
        bool isSrcPolar = (std::abs(srcLat) > m_maxPolarLatDeg);

        for (auto& link : links) {
            auto destIt = m_satelliteMap.find(link.neighborSatId);
            if (destIt == m_satelliteMap.end() || !destIt->second) {
                continue;
            }

            Utils::Position3D destPos = destIt->second->GetOrbitState().GetEcefPosition();
            double dist = Utils::MathUtils::CalculateDistance(srcPos, destPos);
            link.distanceMeters = dist;
            link.propagationDelaySeconds = (dist > 0.0) ? (dist / SPEED_OF_LIGHT_M_S) : 0.0;

            // Only inter-plane links are subject to polar shut-off
            if (link.isInterPlane) {
                double destLat = CalculateLatitudeDeg(destPos);
                bool isDestPolar = (std::abs(destLat) > m_maxPolarLatDeg);
                link.isActive = (!isSrcPolar && !isDestPolar);
            }
        }
    }
}

// ===========================================================================
// Ad-hoc Graph Management (Gateways, UEs, custom links)
// ===========================================================================

void RoutingEngine::AddLink(const std::string& srcNodeId, const std::string& destNodeId,
                            double distanceMeters, bool isInterPlane,
                            double dataRateBps, bool bidirectional) {
    std::lock_guard<std::mutex> lock(m_mutex);
    InvalidateForwardingCache();

    auto InsertOrUpdate = [&](const std::string& u, const std::string& v) {
        auto& links = m_adjacencyList[u];
        auto it = std::find_if(links.begin(), links.end(), [&](const IslLink& l) {
            return l.neighborSatId == v;
        });

        double propDelay = (distanceMeters > 0.0) ? (distanceMeters / SPEED_OF_LIGHT_M_S) : 0.0;
        if (it != links.end()) {
            it->distanceMeters = distanceMeters;
            it->propagationDelaySeconds = propDelay;
            it->dataRateBps = dataRateBps;
            it->isInterPlane = isInterPlane;
            it->isActive = true;
        } else {
            IslLink newLink;
            newLink.neighborSatId = v;
            newLink.distanceMeters = distanceMeters;
            newLink.propagationDelaySeconds = propDelay;
            newLink.dataRateBps = dataRateBps;
            newLink.isInterPlane = isInterPlane;
            newLink.isActive = true;
            links.push_back(newLink);
        }
    };

    InsertOrUpdate(srcNodeId, destNodeId);
    if (bidirectional) {
        InsertOrUpdate(destNodeId, srcNodeId);
    }
}

void RoutingEngine::RemoveLink(const std::string& srcNodeId, const std::string& destNodeId,
                               bool bidirectional) {
    std::lock_guard<std::mutex> lock(m_mutex);
    InvalidateForwardingCache();

    auto EraseFrom = [&](const std::string& u, const std::string& v) {
        auto it = m_adjacencyList.find(u);
        if (it != m_adjacencyList.end()) {
            auto& links = it->second;
            links.erase(std::remove_if(links.begin(), links.end(), [&](const IslLink& l) {
                return l.neighborSatId == v;
            }), links.end());
        }
    };

    EraseFrom(srcNodeId, destNodeId);
    if (bidirectional) {
        EraseFrom(destNodeId, srcNodeId);
    }
}

void RoutingEngine::SetLinkActive(const std::string& srcNodeId, const std::string& destNodeId,
                                  bool active, bool bidirectional) {
    std::lock_guard<std::mutex> lock(m_mutex);
    InvalidateForwardingCache();

    auto SetActiveOn = [&](const std::string& u, const std::string& v) {
        auto it = m_adjacencyList.find(u);
        if (it != m_adjacencyList.end()) {
            for (auto& link : it->second) {
                if (link.neighborSatId == v) {
                    link.isActive = active;
                }
            }
        }
    };

    SetActiveOn(srcNodeId, destNodeId);
    if (bidirectional) {
        SetActiveOn(destNodeId, srcNodeId);
    }
}

void RoutingEngine::TriggerLinkFailure(const std::string& srcNodeId, const std::string& destNodeId,
                                      bool bidirectional) {
    SetLinkActive(srcNodeId, destNodeId, false, bidirectional);
}

void RoutingEngine::RestoreLink(const std::string& srcNodeId, const std::string& destNodeId,
                               bool bidirectional) {
    SetLinkActive(srcNodeId, destNodeId, true, bidirectional);
}

bool RoutingEngine::IsSatelliteInPolarRegion(const std::string& satId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_satelliteMap.find(satId);
    if (it != m_satelliteMap.end() && it->second) {
        Utils::Position3D pos = it->second->GetOrbitState().GetEcefPosition();
        double lat = CalculateLatitudeDeg(pos);
        return (std::abs(lat) > m_maxPolarLatDeg);
    }
    return false;
}

bool RoutingEngine::HasLink(const std::string& srcNodeId, const std::string& destNodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_adjacencyList.find(srcNodeId);
    if (it == m_adjacencyList.end()) {
        return false;
    }
    for (const auto& link : it->second) {
        if (link.neighborSatId == destNodeId) {
            return true;
        }
    }
    return false;
}

std::optional<IslLink> RoutingEngine::GetLink(const std::string& srcNodeId, const std::string& destNodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_adjacencyList.find(srcNodeId);
    if (it != m_adjacencyList.end()) {
        for (const auto& link : it->second) {
            if (link.neighborSatId == destNodeId) {
                return link;
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string> RoutingEngine::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> nodes;
    nodes.reserve(m_adjacencyList.size());
    for (const auto& [nodeId, _] : m_adjacencyList) {
        nodes.push_back(nodeId);
    }
    return nodes;
}

std::vector<IslLink> RoutingEngine::GetLinksForNode(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_adjacencyList.find(nodeId);
    if (it != m_adjacencyList.end()) {
        return it->second;
    }
    return {};
}

NodeTopology RoutingEngine::GetNodeTopology(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    NodeTopology topo;
    topo.nodeId = nodeId;
    auto it = m_adjacencyList.find(nodeId);
    if (it != m_adjacencyList.end()) {
        for (const auto& link : it->second) {
            if (link.isActive) {
                topo.activeLinks.push_back(link);
            }
        }
    }
    return topo;
}

size_t RoutingEngine::GetTotalActiveLinksCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& [_, links] : m_adjacencyList) {
        for (const auto& link : links) {
            if (link.isActive) {
                count++;
            }
        }
    }
    return count;
}

// ===========================================================================
// Congestion & Queuing Delays
// ===========================================================================

void RoutingEngine::SetNodeQueuingDelay(const std::string& nodeId, double queueDelaySeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodeQueuingDelays[nodeId] = queueDelaySeconds;
    InvalidateForwardingCache();
}

double RoutingEngine::GetNodeQueuingDelay(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodeQueuingDelays.find(nodeId);
    if (it != m_nodeQueuingDelays.end()) {
        return it->second;
    }
    return 0.0;
}

void RoutingEngine::ClearQueuingDelays() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodeQueuingDelays.clear();
    InvalidateForwardingCache();
}

void RoutingEngine::InvalidateForwardingCache() {
    m_forwardingTable.clear();
}

// ===========================================================================
// Edge Weight Computation for Dijkstra
// ===========================================================================

double RoutingEngine::DoGetEdgeWeight(const std::string& fromNode, const IslLink& link) const {
    (void)fromNode;
    switch (m_metric) {
        case RoutingMetric::MinHops:
            // Topological metric: uniform edge weight
            return 1.0;

        case RoutingMetric::MinLatency:
            // Physical metric: pure optical propagation delay T_prop = d / c
            return link.propagationDelaySeconds;

        case RoutingMetric::CongestionAware: {
            // Congestion-aware delay: T_prop + T_queue(neighbor)
            double queueDelay = 0.0;
            auto it = m_nodeQueuingDelays.find(link.neighborSatId);
            if (it != m_nodeQueuingDelays.end()) {
                queueDelay = it->second;
            }
            return link.propagationDelaySeconds + queueDelay;
        }
    }
    return link.propagationDelaySeconds;
}

// ===========================================================================
// Phase 2: Shortest-Path Routing Algorithms (Dijkstra)
// ===========================================================================

std::vector<std::string> RoutingEngine::CalculateRoute(const std::string& srcNodeId, const std::string& destNodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return DoCalculateRoute(srcNodeId, destNodeId);
}

std::vector<std::string> RoutingEngine::DoCalculateRoute(const std::string& srcNodeId, const std::string& destNodeId) {
    // Fallback: If topology graph is uninitialized (empty skeleton), preserve original stub behavior
    if (m_adjacencyList.empty()) {
        return {srcNodeId, destNodeId};
    }

    if (srcNodeId == destNodeId) {
        return {srcNodeId};
    }

    // Verify source exists in graph
    if (m_adjacencyList.find(srcNodeId) == m_adjacencyList.end()) {
        return {};
    }

    // Min-priority queue: stores (accumulated_cost, node_id)
    using CostNode = std::pair<double, std::string>;
    std::priority_queue<CostNode, std::vector<CostNode>, std::greater<CostNode>> pq;

    std::unordered_map<std::string, double> minCost;
    std::unordered_map<std::string, std::string> prevNode;

    minCost[srcNodeId] = 0.0;
    pq.emplace(0.0, srcNodeId);

    bool reachedDestination = false;

    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();

        // If a shorter path to u has already been processed, skip stale entry
        if (currentCost > minCost[u]) {
            continue;
        }

        // Destination reached with minimal cost
        if (u == destNodeId) {
            reachedDestination = true;
            break;
        }

        auto it = m_adjacencyList.find(u);
        if (it == m_adjacencyList.end()) {
            continue;
        }

        // Explore all outgoing active cross-links
        for (const auto& link : it->second) {
            if (!link.isActive) {
                continue; // Skip deactivated or broken links (e.g. polar shutoff or failure)
            }

            double edgeCost = DoGetEdgeWeight(u, link);
            double newCost = currentCost + edgeCost;

            const std::string& v = link.neighborSatId;
            auto costIt = minCost.find(v);
            if (costIt == minCost.end() || newCost < costIt->second) {
                minCost[v] = newCost;
                prevNode[v] = u;
                pq.emplace(newCost, v);
            }
        }
    }

    if (!reachedDestination) {
        return {}; // No operational pathway connects src to dest
    }

    // Backtrack from destNodeId to srcNodeId using predecessor pointers
    std::vector<std::string> path;
    for (std::string curr = destNodeId; curr != srcNodeId; curr = prevNode[curr]) {
        path.push_back(curr);
    }
    path.push_back(srcNodeId);
    std::reverse(path.begin(), path.end());

    return path;
}

std::string RoutingEngine::ResolveNextHop(const std::string& currentNodeId, const std::string& destNodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Fallback: If topology graph is uninitialized (empty skeleton), return destNodeId
    if (m_adjacencyList.empty()) {
        return destNodeId;
    }

    if (currentNodeId == destNodeId) {
        return currentNodeId;
    }

    // Check forwarding table cache first for O(1) forwarding lookup
    auto srcIt = m_forwardingTable.find(currentNodeId);
    if (srcIt != m_forwardingTable.end()) {
        auto destIt = srcIt->second.find(destNodeId);
        if (destIt != srcIt->second.end()) {
            return destIt->second;
        }
    }

    // Route not yet cached in forwarding table: run Dijkstra path-finding
    auto route = DoCalculateRoute(currentNodeId, destNodeId);
    if (route.size() >= 2) {
        // Immediate next hop is the second element in the route [curr, nextHop, ...]
        std::string nextHop = route[1];

        // Cache next hop for O(1) subsequent packet forwarding
        m_forwardingTable[currentNodeId][destNodeId] = nextHop;

        // Populate forwarding entries along the entire discovered path
        for (size_t i = 0; i + 1 < route.size(); ++i) {
            m_forwardingTable[route[i]][destNodeId] = route[i + 1];
        }

        return nextHop;
    }

    return ""; // Unreachable destination
}

bool RoutingEngine::IsRouteAvailable(const std::string& srcNodeId, const std::string& destNodeId) {
    auto route = CalculateRoute(srcNodeId, destNodeId);
    return !route.empty();
}

// ===========================================================================
// Route Analysis Helpers
// ===========================================================================

double RoutingEngine::CalculateRouteDistance(const std::vector<std::string>& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (path.size() < 2) {
        return 0.0;
    }

    double totalDist = 0.0;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto it = m_adjacencyList.find(path[i]);
        if (it != m_adjacencyList.end()) {
            for (const auto& link : it->second) {
                if (link.neighborSatId == path[i + 1]) {
                    totalDist += link.distanceMeters;
                    break;
                }
            }
        }
    }
    return totalDist;
}

double RoutingEngine::CalculateRouteLatency(const std::vector<std::string>& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (path.size() < 2) {
        return 0.0;
    }

    double totalLatency = 0.0;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto it = m_adjacencyList.find(path[i]);
        if (it != m_adjacencyList.end()) {
            for (const auto& link : it->second) {
                if (link.neighborSatId == path[i + 1]) {
                    totalLatency += link.propagationDelaySeconds;
                    break;
                }
            }
        }
    }
    return totalLatency;
}

double RoutingEngine::CalculateRouteCost(const std::vector<std::string>& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (path.size() < 2) {
        return 0.0;
    }

    double totalCost = 0.0;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        auto it = m_adjacencyList.find(path[i]);
        if (it != m_adjacencyList.end()) {
            for (const auto& link : it->second) {
                if (link.neighborSatId == path[i + 1]) {
                    totalCost += DoGetEdgeWeight(path[i], link);
                    break;
                }
            }
        }
    }
    return totalCost;
}

void RoutingEngine::ClearTopology() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_adjacencyList.clear();
    m_forwardingTable.clear();
    m_nodeQueuingDelays.clear();
    m_satellites.clear();
    m_satelliteMap.clear();
    m_planes = 0;
    m_satsPerPlane = 0;
}

} // namespace NtnSim::Routing
