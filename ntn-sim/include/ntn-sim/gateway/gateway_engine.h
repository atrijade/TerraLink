#pragma once

#include "ntn-sim/gateway/gateway.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace NtnSim::Orbit {
class OrbitEngine;
}
namespace NtnSim::Coverage {
class CoverageEngine;
}
namespace NtnSim::Signal {
class SignalEngine;
}
namespace NtnSim::Routing {
class RoutingEngine;
}
namespace NtnSim::Packet {
class PacketEngine;
class Packet;
}
namespace NtnSim::Metrics {
class MetricsEngine;
}
namespace NtnSim::Core {
class EventScheduler;
}

namespace NtnSim::Gateway {

/**
 * @brief Manages the gateway fleet, coordinates feeder link tracking with OrbitEngine,
 *        computes horizon mask visibility via CoverageEngine, manages 3GPP feeder link budgets,
 *        and triggers dynamic handovers with RoutingEngine and MetricsEngine.
 */
class GatewayEngine {
public:
    GatewayEngine();
    explicit GatewayEngine(Metrics::MetricsEngine* metricsEngine, Routing::RoutingEngine* routingEngine = nullptr);
    ~GatewayEngine();

    // =========================================================================
    // Fleet Management
    // =========================================================================
    void AddGateway(std::shared_ptr<Gateway> gateway);
    void AddGateway(Gateway* gateway);
    bool RemoveGateway(const std::string& gatewayId);
    std::shared_ptr<Gateway> GetGateway(const std::string& gatewayId) const;
    Gateway* GetGatewayRaw(const std::string& gatewayId) const;
    std::vector<std::shared_ptr<Gateway>> GetAllGateways() const;
    size_t GetGatewayCount() const;
    void ClearGateways();

    // =========================================================================
    // Engine Dependency Injection
    // =========================================================================
    void SetOrbitEngine(Orbit::OrbitEngine* orbitEngine);
    void SetCoverageEngine(Coverage::CoverageEngine* coverageEngine);
    void SetSignalEngine(Signal::SignalEngine* signalEngine);
    void SetRoutingEngine(Routing::RoutingEngine* routingEngine);
    void SetPacketEngine(Packet::PacketEngine* packetEngine);
    void SetMetricsEngine(Metrics::MetricsEngine* metricsEngine);

    Orbit::OrbitEngine* GetOrbitEngine() const;
    Coverage::CoverageEngine* GetCoverageEngine() const;
    Signal::SignalEngine* GetSignalEngine() const;
    Routing::RoutingEngine* GetRoutingEngine() const;
    Packet::PacketEngine* GetPacketEngine() const;
    Metrics::MetricsEngine* GetMetricsEngine() const;

    // =========================================================================
    // Step 3: Feeder Link Handover & Tracking Loop
    // =========================================================================
    /**
     * @brief Evaluates line-of-sight geometry and horizon masks for all gateways,
     *        switching feeder tracking to the next rising satellite and updating the
     *        RoutingEngine topology graph dynamically.
     */
    void UpdateFeederLinks(double currentTime);

    // =========================================================================
    // Step 4: Traffic Injection & Telemetry
    // =========================================================================
    /**
     * @brief Injects an uplink packet from a designated gateway towards a destination node.
     */
    std::shared_ptr<Packet::Packet> InjectUplinkPacket(
        const std::string& gatewayId,
        uint64_t packetId,
        uint32_t sizeBytes,
        double currentTime,
        const std::string& destinationId,
        Core::EventScheduler* scheduler = nullptr);

    /**
     * @brief Records telemetry from all gateways into MetricsEngine.
     */
    void RecordMetrics(double currentTime);

private:
    std::unordered_map<std::string, std::shared_ptr<Gateway>> m_gateways;
    std::unordered_map<std::string, Gateway*> m_gatewaysRaw;

    Orbit::OrbitEngine* m_orbitEngine{nullptr};
    Coverage::CoverageEngine* m_coverageEngine{nullptr};
    Signal::SignalEngine* m_signalEngine{nullptr};
    Routing::RoutingEngine* m_routingEngine{nullptr};
    Packet::PacketEngine* m_packetEngine{nullptr};
    Metrics::MetricsEngine* m_metricsEngine{nullptr};

    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Gateway
