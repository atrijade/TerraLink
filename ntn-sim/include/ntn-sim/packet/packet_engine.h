#pragma once

#include "ntn-sim/packet/packet.h"
#include "ntn-sim/metrics/kpi_types.h"
#include "ntn-sim/signal/signal_info.h"
#include "ntn-sim/utils/random_generator.h"

#include <queue>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

namespace NtnSim::Metrics {
class MetricsEngine;
}

namespace NtnSim::Core {
class EventScheduler;
}

namespace NtnSim::Routing {
class RoutingEngine;
}

namespace NtnSim::Gateway {
class Gateway;
}

namespace NtnSim::Packet {

/**
 * @brief Traffic generation distribution pattern.
 */
enum class TrafficPattern {
    CBR,        ///< Constant Bit Rate (fixed-interval packet generation: 1 / rate)
    Poisson     ///< Poisson / Exponential inter-arrival distribution (bursty traffic)
};

/**
 * @brief Configuration model for traffic flow generation.
 *        Customizable via UI / REST API or configuration files.
 */
struct TrafficFlowConfig {
    std::string flowId{"default-flow"};
    std::string sourceId{"user-0"};
    std::string destinationId{"gw-0"};
    TrafficPattern pattern{TrafficPattern::CBR};
    double packetRate{100.0};       ///< Generation rate in packets per second (lambda)
    uint32_t packetSizeBytes{1500}; ///< PDU size in bytes
    uint32_t qosClass{2};           ///< 0=Control/Signaling, 1=Voice/Video, 2=Best-Effort
    uint32_t ttl{64};               ///< Initial hop limit (Time-To-Live)
    double maxLifetimeSeconds{2.0}; ///< Lifetime timeout
    double startTime{0.0};          ///< Simulation time when flow begins (s)
    double stopTime{3600.0};        ///< Simulation time when flow stops (s)
};

/**
 * @brief Configuration parameters for a node transmission queue.
 */
struct QueueConfig {
    size_t maxPackets{1000};                     ///< Maximum packet capacity before DropTail
    size_t maxSizeBytes{10 * 1024 * 1024};       ///< Maximum byte capacity (default 10 MB)
};

/**
 * @brief Dynamic runtime state for a node packet buffer.
 */
struct QueueState {
    QueueConfig config;
    std::queue<std::shared_ptr<Packet>> packets;
    size_t currentSizeBytes{0};
    size_t droppedPacketsCount{0};
    double lastQueueDelaySeconds{0.0};
};

/**
 * @brief Detailed breakdown of hop latency components:
 *        Total Latency = T_queue + T_tx + T_prop + T_proc
 */
struct HopLatencyBreakdown {
    double queueDelaySeconds{0.0};     ///< T_queue: Waiting time in transmission queue
    double txDelaySeconds{0.0};        ///< T_tx: Packet size / Data rate
    double propDelaySeconds{0.0};      ///< T_prop: Distance / Speed of light
    double procDelaySeconds{0.0};      ///< T_proc: Node processing/switching delay
    double totalHopLatencySeconds{0.0};///< Sum of all delays
};

/**
 * @brief Outcome of a single hop transmission.
 */
struct TransmissionResult {
    bool success{false};
    Metrics::PacketDropReason dropReason{Metrics::PacketDropReason::QueueOverflow};
    HopLatencyBreakdown latencyBreakdown;
    double arrivalTime{0.0};
};

/**
 * @brief Manages queuing, traffic generation, buffer limits, physical latency modeling,
 *        DropTail policies, multi-hop routing integration, and event-driven telemetry hooks.
 */
class PacketEngine {
public:
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;

    PacketEngine();
    explicit PacketEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine);
    explicit PacketEngine(Metrics::MetricsEngine* metricsEngine);
    PacketEngine(Metrics::MetricsEngine* metricsEngine, Routing::RoutingEngine* routingEngine);
    PacketEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine, std::shared_ptr<Routing::RoutingEngine> routingEngine);
    ~PacketEngine();

    // =========================================================================
    // Phase 1 & 2: Packet Model & Buffer Management
    // =========================================================================

    /**
     * @brief Creates a new network packet PDU.
     */
    std::shared_ptr<Packet> GeneratePacket(uint64_t packetId, uint32_t sizeBytes,
                                          double creationTime, const std::string& srcNodeId,
                                          const std::string& destNodeId,
                                          uint32_t qosClass = 2, uint32_t ttl = 64,
                                          double maxLifetimeSeconds = 2.0);

    /**
     * @brief Configures buffer capacity limits for a specific queue.
     */
    void ConfigureQueue(const std::string& queueId, size_t maxPackets, size_t maxSizeBytes = 10 * 1024 * 1024);

    /**
     * @brief Configures buffer capacity limits using a QueueConfig struct.
     */
    void ConfigureQueue(const std::string& queueId, const QueueConfig& config);

    /**
     * @brief Gets current configuration of a queue.
     */
    QueueConfig GetQueueConfig(const std::string& queueId) const;

    /**
     * @brief Enqueues a packet into a node queue. Applies DropTail policy if queue limit is exceeded.
     */
    bool EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet, double enqueueTime = 0.0);

    /**
     * @brief Pops the front packet from a queue and calculates queue latency.
     */
    std::shared_ptr<Packet> DequeuePacket(const std::string& queueId, double dequeueTime = 0.0);

    size_t GetQueueSize(const std::string& queueId) const;
    size_t GetQueueSizeBytes(const std::string& queueId) const;
    size_t GetDroppedPacketCount(const std::string& queueId) const;
    double GetLastQueueDelay(const std::string& queueId) const;
    void ClearQueue(const std::string& queueId);

    // =========================================================================
    // Phase 3: Traffic Generation Models (CBR & Poisson)
    // =========================================================================

    void AddTrafficFlow(const TrafficFlowConfig& config);
    bool RemoveTrafficFlow(const std::string& flowId);
    std::optional<TrafficFlowConfig> GetTrafficFlow(const std::string& flowId) const;
    std::vector<TrafficFlowConfig> GetAllTrafficFlows() const;
    void ClearTrafficFlows();

    /**
     * @brief Schedules recurring packet generation events for a specific flow into EventScheduler.
     */
    void ScheduleTrafficFlow(const TrafficFlowConfig& config, Core::EventScheduler& scheduler);

    /**
     * @brief Schedules all registered traffic flows into EventScheduler.
     */
    void StartTrafficGeneration(Core::EventScheduler& scheduler);

    /**
     * @brief Computes next inter-arrival interval according to Poisson / Exponential distribution.
     */
    double GetNextPoissonInterval(double packetRate);

    // =========================================================================
    // Phase 4: Transmission & End-to-End Latency Modeling
    // =========================================================================

    /**
     * @brief Calculates transmission delay T_tx = size(bits) / dataRate(bps).
     */
    static double CalculateTxDelay(uint32_t packetSizeBytes, double dataRateBps);

    /**
     * @brief Calculates propagation delay T_prop = distance / c.
     */
    static double CalculatePropDelay(double distanceMeters);

    /**
     * @brief Calculates full latency breakdown for a hop: T_queue + T_tx + T_prop + T_proc.
     */
    HopLatencyBreakdown CalculateHopLatency(uint32_t packetSizeBytes, double dataRateBps,
                                           double distanceMeters, double queueDelaySeconds = 0.0,
                                           double procDelaySeconds = 0.00001) const;

    /**
     * @brief Transmits a packet across a physical link hop with SINR and TTL checks.
     */
    TransmissionResult TransmitHop(std::shared_ptr<Packet> packet,
                                  const std::string& nextNodeId,
                                  double dataRateBps,
                                  double distanceMeters,
                                  double sinrDb,
                                  double currentTime,
                                  double minSinrThresholdDb = 0.0,
                                  double procDelaySeconds = 0.00001);

    /**
     * @brief Transmits a packet using SignalInfo link budget directly.
     */
    TransmissionResult TransmitHop(std::shared_ptr<Packet> packet,
                                  const std::string& nextNodeId,
                                  double dataRateBps,
                                  const Signal::SignalInfo& signalInfo,
                                  double currentTime,
                                  double minSinrThresholdDb = 0.0,
                                  double procDelaySeconds = 0.00001);

    // =========================================================================
    // Telemetry & Metrics Hook
    // =========================================================================

    void SetMetricsEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine);
    void SetMetricsEngine(Metrics::MetricsEngine* metricsEngine);
    Metrics::MetricsEngine* GetMetricsEngine() const;

    /**
     * @brief Delivers a packet at its destination/gateway and records reception KPI.
     */
    void DeliverPacket(std::shared_ptr<Packet> packet, double deliveryTime);

    /**
     * @brief Drops a packet, logs the reason, and records drop KPI.
     */
    void DropPacket(std::shared_ptr<Packet> packet, Metrics::PacketDropReason reason, double timestamp);

    // =========================================================================
    // Gateway Packet Sink Integration
    // =========================================================================
    void RegisterGateway(Gateway::Gateway* gateway);
    void RegisterGateway(std::shared_ptr<Gateway::Gateway> gateway);
    void UnregisterGateway(const std::string& gatewayId);
    Gateway::Gateway* GetGateway(const std::string& gatewayId) const;

    // =========================================================================
    // Multi-Hop Routing Engine Integration & Discrete-Event Forwarding
    // =========================================================================

    void SetRoutingEngine(std::shared_ptr<Routing::RoutingEngine> routingEngine);
    void SetRoutingEngine(Routing::RoutingEngine* routingEngine);
    Routing::RoutingEngine* GetRoutingEngine() const;

    /**
     * @brief Initiates end-to-end multi-hop packet routing from currentNodeId towards destination.
     *        Queries the RoutingEngine for next hop, checks optical ISL parameters,
     *        and schedules hop-by-hop transmission in EventScheduler until reaching destination.
     */
    bool RoutePacketMultiHop(std::shared_ptr<Packet> packet,
                            const std::string& currentNodeId,
                            double currentTime,
                            Core::EventScheduler& scheduler,
                            double defaultDataRateBps = 10.0e9,
                            double sinrDb = 30.0,
                            double minSinrThresholdDb = 0.0,
                            double procDelaySeconds = 0.00001);

    /**
     * @brief Handles intermediate hop arrival: queries RoutingEngine, detects link failures,
     *        reroutes if necessary, and schedules subsequent hop or final delivery.
     */
    bool ForwardNextHop(std::shared_ptr<Packet> packet,
                        const std::string& currentNodeId,
                        double currentTime,
                        Core::EventScheduler& scheduler,
                        double defaultDataRateBps = 10.0e9,
                        double sinrDb = 30.0,
                        double minSinrThresholdDb = 0.0,
                        double procDelaySeconds = 0.00001);

    /**
     * @brief Schedules an event-driven hop transmission in the EventScheduler.
     * @param autoForwardMultiHop If true and RoutingEngine is configured, subsequent hops are automatically scheduled upon arrival.
     */
    bool ScheduleHopTransmission(std::shared_ptr<Packet> packet,
                                const std::string& nextNodeId,
                                double dataRateBps,
                                double distanceMeters,
                                double sinrDb,
                                double currentTime,
                                Core::EventScheduler& scheduler,
                                double minSinrThresholdDb = 0.0,
                                double procDelaySeconds = 0.00001,
                                bool autoForwardMultiHop = false);

    // Telemetry getters
    uint64_t GetTotalPacketsSent() const;
    uint64_t GetTotalPacketsDelivered() const;
    uint64_t GetTotalPacketsDropped() const;

private:
    uint64_t GetNextPacketIdInternal();
    void ScheduleTrafficFlowInternal(const TrafficFlowConfig& config, Core::EventScheduler& scheduler);

    std::unordered_map<std::string, QueueState> m_queues;
    std::vector<TrafficFlowConfig> m_trafficFlows;

    Metrics::MetricsEngine* m_metricsEngineRaw{nullptr};
    std::shared_ptr<Metrics::MetricsEngine> m_metricsEngineShared{nullptr};

    Routing::RoutingEngine* m_routingEngineRaw{nullptr};
    std::shared_ptr<Routing::RoutingEngine> m_routingEngineShared{nullptr};

    std::unordered_map<std::string, Gateway::Gateway*> m_gatewaysRaw;
    std::unordered_map<std::string, std::shared_ptr<Gateway::Gateway>> m_gatewaysShared;

    Utils::RandomGenerator m_rng;
    uint64_t m_nextPacketId{1};
    uint64_t m_totalSent{0};
    uint64_t m_totalDelivered{0};
    uint64_t m_totalDropped{0};

    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Packet
