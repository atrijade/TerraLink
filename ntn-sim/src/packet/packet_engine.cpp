#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/core/event.h"
#include <cmath>
#include <algorithm>

namespace NtnSim::Packet {

PacketEngine::PacketEngine() {}

PacketEngine::PacketEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine)
    : m_metricsEngineRaw(metricsEngine.get()),
      m_metricsEngineShared(std::move(metricsEngine)) {}

PacketEngine::PacketEngine(Metrics::MetricsEngine* metricsEngine)
    : m_metricsEngineRaw(metricsEngine) {}

PacketEngine::PacketEngine(Metrics::MetricsEngine* metricsEngine, Routing::RoutingEngine* routingEngine)
    : m_metricsEngineRaw(metricsEngine),
      m_routingEngineRaw(routingEngine) {}

PacketEngine::PacketEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine,
                           std::shared_ptr<Routing::RoutingEngine> routingEngine)
    : m_metricsEngineRaw(metricsEngine.get()),
      m_metricsEngineShared(std::move(metricsEngine)),
      m_routingEngineRaw(routingEngine.get()),
      m_routingEngineShared(std::move(routingEngine)) {}

PacketEngine::~PacketEngine() {}

// =========================================================================
// Phase 1 & 2: Packet Model & Buffer Management
// =========================================================================

uint64_t PacketEngine::GetNextPacketIdInternal() {
    return m_nextPacketId++;
}

std::shared_ptr<Packet> PacketEngine::GeneratePacket(uint64_t packetId, uint32_t sizeBytes,
                                                    double creationTime, const std::string& srcNodeId,
                                                    const std::string& destNodeId,
                                                    uint32_t qosClass, uint32_t ttl,
                                                    double maxLifetimeSeconds) {
    uint64_t assignedId = packetId;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (assignedId == 0) {
            assignedId = m_nextPacketId++;
        } else if (assignedId >= m_nextPacketId) {
            m_nextPacketId = assignedId + 1;
        }
        m_totalSent++;
        if (m_metricsEngineRaw) {
            m_metricsEngineRaw->RecordPacketSent(assignedId, sizeBytes, creationTime);
        }
    }

    return std::make_shared<Packet>(assignedId, sizeBytes, creationTime, srcNodeId,
                                    destNodeId, qosClass, ttl, maxLifetimeSeconds);
}

void PacketEngine::ConfigureQueue(const std::string& queueId, size_t maxPackets, size_t maxSizeBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues[queueId].config.maxPackets = maxPackets;
    m_queues[queueId].config.maxSizeBytes = maxSizeBytes;
}

void PacketEngine::ConfigureQueue(const std::string& queueId, const QueueConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues[queueId].config = config;
}

QueueConfig PacketEngine::GetQueueConfig(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.config;
    }
    return QueueConfig{};
}

bool PacketEngine::EnqueuePacket(const std::string& queueId, std::shared_ptr<Packet> packet, double enqueueTime) {
    if (!packet) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto& qState = m_queues[queueId];

    // Check packet count limit (DropTail policy)
    if (qState.packets.size() >= qState.config.maxPackets) {
        qState.droppedPacketsCount++;
        m_totalDropped++;
        if (m_metricsEngineRaw) {
            m_metricsEngineRaw->RecordPacketDropped(packet->GetId(), Metrics::PacketDropReason::QueueOverflow, enqueueTime);
        }
        return false;
    }

    // Check byte size limit (DropTail policy)
    if (qState.currentSizeBytes + packet->GetSizeBytes() > qState.config.maxSizeBytes) {
        qState.droppedPacketsCount++;
        m_totalDropped++;
        if (m_metricsEngineRaw) {
            m_metricsEngineRaw->RecordPacketDropped(packet->GetId(), Metrics::PacketDropReason::QueueOverflow, enqueueTime);
        }
        return false;
    }

    packet->SetEnqueueTime(enqueueTime);
    qState.packets.push(packet);
    qState.currentSizeBytes += packet->GetSizeBytes();
    return true;
}

std::shared_ptr<Packet> PacketEngine::DequeuePacket(const std::string& queueId, double dequeueTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it == m_queues.end() || it->second.packets.empty()) {
        return nullptr;
    }

    auto& qState = it->second;
    auto frontPkt = qState.packets.front();
    qState.packets.pop();

    if (frontPkt->GetSizeBytes() <= qState.currentSizeBytes) {
        qState.currentSizeBytes -= frontPkt->GetSizeBytes();
    } else {
        qState.currentSizeBytes = 0;
    }

    frontPkt->SetDepartureTime(dequeueTime);
    double queueDelay = 0.0;
    if (dequeueTime >= frontPkt->GetEnqueueTime()) {
        queueDelay = dequeueTime - frontPkt->GetEnqueueTime();
    }
    qState.lastQueueDelaySeconds = queueDelay;

    return frontPkt;
}

size_t PacketEngine::GetQueueSize(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.packets.size();
    }
    return 0;
}

size_t PacketEngine::GetQueueSizeBytes(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.currentSizeBytes;
    }
    return 0;
}

size_t PacketEngine::GetDroppedPacketCount(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.droppedPacketsCount;
    }
    return 0;
}

double PacketEngine::GetLastQueueDelay(const std::string& queueId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        return it->second.lastQueueDelaySeconds;
    }
    return 0.0;
}

void PacketEngine::ClearQueue(const std::string& queueId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end()) {
        std::queue<std::shared_ptr<Packet>> emptyQueue;
        std::swap(it->second.packets, emptyQueue);
        it->second.currentSizeBytes = 0;
    }
}

// =========================================================================
// Phase 3: Traffic Generation Models (CBR & Poisson)
// =========================================================================

void PacketEngine::AddTrafficFlow(const TrafficFlowConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_trafficFlows.begin(), m_trafficFlows.end(), [&config](const auto& f) {
        return f.flowId == config.flowId;
    });
    if (it != m_trafficFlows.end()) {
        *it = config;
    } else {
        m_trafficFlows.push_back(config);
    }
}

bool PacketEngine::RemoveTrafficFlow(const std::string& flowId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_trafficFlows.begin(), m_trafficFlows.end(), [&flowId](const auto& f) {
        return f.flowId == flowId;
    });
    if (it != m_trafficFlows.end()) {
        m_trafficFlows.erase(it);
        return true;
    }
    return false;
}

std::optional<TrafficFlowConfig> PacketEngine::GetTrafficFlow(const std::string& flowId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_trafficFlows.begin(), m_trafficFlows.end(), [&flowId](const auto& f) {
        return f.flowId == flowId;
    });
    if (it != m_trafficFlows.end()) {
        return *it;
    }
    return std::nullopt;
}

std::vector<TrafficFlowConfig> PacketEngine::GetAllTrafficFlows() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trafficFlows;
}

void PacketEngine::ClearTrafficFlows() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trafficFlows.clear();
}

double PacketEngine::GetNextPoissonInterval(double packetRate) {
    if (packetRate <= 0.0) {
        return 1.0;
    }
    double u = m_rng.GetUniformDouble(1e-9, 1.0);
    return -std::log(u) / packetRate;
}

void PacketEngine::ScheduleTrafficFlow(const TrafficFlowConfig& config, Core::EventScheduler& scheduler) {
    AddTrafficFlow(config);
    ScheduleTrafficFlowInternal(config, scheduler);
}

void PacketEngine::StartTrafficGeneration(Core::EventScheduler& scheduler) {
    std::vector<TrafficFlowConfig> flowsCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        flowsCopy = m_trafficFlows;
    }
    for (const auto& flow : flowsCopy) {
        ScheduleTrafficFlowInternal(flow, scheduler);
    }
}

void PacketEngine::ScheduleTrafficFlowInternal(const TrafficFlowConfig& config, Core::EventScheduler& scheduler) {
    auto scheduleStep = std::make_shared<std::function<void(double)>>();

    *scheduleStep = [this, config, &scheduler, scheduleStep](double currentSimTime) {
        if (currentSimTime > config.stopTime) {
            return;
        }

        // 1. Generate packet
        auto pkt = GeneratePacket(0, config.packetSizeBytes, currentSimTime,
                                  config.sourceId, config.destinationId,
                                  config.qosClass, config.ttl, config.maxLifetimeSeconds);

        // 2. Enqueue into source node's queue
        EnqueuePacket(config.sourceId, pkt, currentSimTime);

        // 3. Compute next generation time
        double interval = 0.0;
        if (config.pattern == TrafficPattern::CBR) {
            interval = (config.packetRate > 0.0) ? (1.0 / config.packetRate) : 1.0;
        } else {
            interval = GetNextPoissonInterval(config.packetRate);
        }

        double nextTime = currentSimTime + interval;
        if (nextTime <= config.stopTime) {
            scheduler.ScheduleEvent(nextTime, 5, Core::EventType::GeneratePacket,
                                    [scheduleStep, nextTime]() {
                                        (*scheduleStep)(nextTime);
                                    });
        }
    };

    // Schedule the initial packet generation
    scheduler.ScheduleEvent(config.startTime, 5, Core::EventType::GeneratePacket,
                            [scheduleStep, startTime = config.startTime]() {
                                (*scheduleStep)(startTime);
                            });
}

// =========================================================================
// Phase 4: Transmission & End-to-End Latency Modeling
// =========================================================================

double PacketEngine::CalculateTxDelay(uint32_t packetSizeBytes, double dataRateBps) {
    if (dataRateBps <= 0.0) {
        return 0.0;
    }
    return (static_cast<double>(packetSizeBytes) * 8.0) / dataRateBps;
}

double PacketEngine::CalculatePropDelay(double distanceMeters) {
    if (distanceMeters <= 0.0) {
        return 0.0;
    }
    return distanceMeters / SPEED_OF_LIGHT_M_S;
}

HopLatencyBreakdown PacketEngine::CalculateHopLatency(uint32_t packetSizeBytes, double dataRateBps,
                                                     double distanceMeters, double queueDelaySeconds,
                                                     double procDelaySeconds) const {
    HopLatencyBreakdown breakdown;
    breakdown.queueDelaySeconds = (queueDelaySeconds >= 0.0) ? queueDelaySeconds : 0.0;
    breakdown.txDelaySeconds = CalculateTxDelay(packetSizeBytes, dataRateBps);
    breakdown.propDelaySeconds = CalculatePropDelay(distanceMeters);
    breakdown.procDelaySeconds = (procDelaySeconds >= 0.0) ? procDelaySeconds : 0.0;
    breakdown.totalHopLatencySeconds = breakdown.queueDelaySeconds +
                                       breakdown.txDelaySeconds +
                                       breakdown.propDelaySeconds +
                                       breakdown.procDelaySeconds;
    return breakdown;
}

TransmissionResult PacketEngine::TransmitHop(std::shared_ptr<Packet> packet,
                                            const std::string& nextNodeId,
                                            double dataRateBps,
                                            double distanceMeters,
                                            double sinrDb,
                                            double currentTime,
                                            double minSinrThresholdDb,
                                            double procDelaySeconds) {
    TransmissionResult result;
    if (!packet) {
        result.success = false;
        return result;
    }

    // 1. Check TTL decrement and expiry
    if (!packet->DecrementTtl() || packet->IsExpired(currentTime)) {
        result.success = false;
        result.dropReason = Metrics::PacketDropReason::TtlExpired;
        DropPacket(packet, Metrics::PacketDropReason::TtlExpired, currentTime);
        return result;
    }

    // 2. Check Radio Channel SINR threshold
    if (sinrDb < minSinrThresholdDb) {
        result.success = false;
        result.dropReason = Metrics::PacketDropReason::RadioChannelLoss;
        DropPacket(packet, Metrics::PacketDropReason::RadioChannelLoss, currentTime);
        return result;
    }

    // 3. Calculate latency components
    double queueDelay = 0.0;
    if (packet->GetEnqueueTime() > 0.0 && currentTime >= packet->GetEnqueueTime()) {
        queueDelay = currentTime - packet->GetEnqueueTime();
    }

    result.latencyBreakdown = CalculateHopLatency(packet->GetSizeBytes(), dataRateBps,
                                                 distanceMeters, queueDelay, procDelaySeconds);

    double arrivalTime = currentTime + result.latencyBreakdown.txDelaySeconds +
                         result.latencyBreakdown.propDelaySeconds +
                         result.latencyBreakdown.procDelaySeconds;
    result.arrivalTime = arrivalTime;

    // Check if packet expires during transit before arriving
    if (packet->IsExpired(arrivalTime)) {
        result.success = false;
        result.dropReason = Metrics::PacketDropReason::TtlExpired;
        DropPacket(packet, Metrics::PacketDropReason::TtlExpired, arrivalTime);
        return result;
    }

    // Advance packet state
    packet->AddHop(nextNodeId);
    packet->SetDepartureTime(currentTime);

    result.success = true;
    return result;
}

TransmissionResult PacketEngine::TransmitHop(std::shared_ptr<Packet> packet,
                                            const std::string& nextNodeId,
                                            double dataRateBps,
                                            const Signal::SignalInfo& signalInfo,
                                            double currentTime,
                                            double minSinrThresholdDb,
                                            double procDelaySeconds) {
    double distanceMeters = signalInfo.GetLatencySeconds() * SPEED_OF_LIGHT_M_S;
    return TransmitHop(packet, nextNodeId, dataRateBps, distanceMeters,
                       signalInfo.GetSinrDb(), currentTime, minSinrThresholdDb, procDelaySeconds);
}

// =========================================================================
// Telemetry & Metrics Hook
// =========================================================================

void PacketEngine::SetMetricsEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsEngineShared = std::move(metricsEngine);
    m_metricsEngineRaw = m_metricsEngineShared.get();
}

void PacketEngine::SetMetricsEngine(Metrics::MetricsEngine* metricsEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsEngineShared.reset();
    m_metricsEngineRaw = metricsEngine;
}

Metrics::MetricsEngine* PacketEngine::GetMetricsEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metricsEngineRaw;
}

void PacketEngine::DeliverPacket(std::shared_ptr<Packet> packet, double deliveryTime) {
    if (!packet) {
        return;
    }
    packet->SetArrivalTime(deliveryTime);
    double latency = packet->GetEndToEndLatency();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_totalDelivered++;
    if (m_metricsEngineRaw) {
        m_metricsEngineRaw->RecordPacketReceived(packet->GetId(), latency, deliveryTime);
    }
}

void PacketEngine::DropPacket(std::shared_ptr<Packet> packet, Metrics::PacketDropReason reason, double timestamp) {
    if (!packet) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_totalDropped++;
    if (m_metricsEngineRaw) {
        m_metricsEngineRaw->RecordPacketDropped(packet->GetId(), reason, timestamp);
    }
}

// =========================================================================
// Multi-Hop Routing Engine Integration & Discrete-Event Forwarding
// =========================================================================

void PacketEngine::SetRoutingEngine(std::shared_ptr<Routing::RoutingEngine> routingEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routingEngineShared = std::move(routingEngine);
    m_routingEngineRaw = m_routingEngineShared.get();
}

void PacketEngine::SetRoutingEngine(Routing::RoutingEngine* routingEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routingEngineShared.reset();
    m_routingEngineRaw = routingEngine;
}

Routing::RoutingEngine* PacketEngine::GetRoutingEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_routingEngineRaw;
}

bool PacketEngine::RoutePacketMultiHop(std::shared_ptr<Packet> packet,
                                       const std::string& currentNodeId,
                                       double currentTime,
                                       Core::EventScheduler& scheduler,
                                       double defaultDataRateBps,
                                       double sinrDb,
                                       double minSinrThresholdDb,
                                       double procDelaySeconds) {
    if (!packet) {
        return false;
    }

    if (packet->IsExpired(currentTime) || packet->GetTtl() == 0) {
        DropPacket(packet, Metrics::PacketDropReason::TtlExpired, currentTime);
        return false;
    }

    const std::string& destId = packet->GetDestinationNodeId();
    if (currentNodeId == destId) {
        DeliverPacket(packet, currentTime);
        return true;
    }

    Routing::RoutingEngine* router = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        router = m_routingEngineRaw;
    }

    if (!router) {
        return false;
    }

    std::string nextHop = router->ResolveNextHop(currentNodeId, destId);
    if (nextHop.empty()) {
        DropPacket(packet, Metrics::PacketDropReason::NoRoute, currentTime);
        return false;
    }

    double hopDist = 1000.0e3;
    double hopRate = defaultDataRateBps;

    auto linkOpt = router->GetLink(currentNodeId, nextHop);
    if (linkOpt.has_value()) {
        if (!linkOpt->isActive) {
            DropPacket(packet, Metrics::PacketDropReason::NoRoute, currentTime);
            return false;
        }
        hopDist = linkOpt->distanceMeters;
        hopRate = linkOpt->dataRateBps;
    }

    return ScheduleHopTransmission(packet, nextHop, hopRate, hopDist, sinrDb, currentTime,
                                  scheduler, minSinrThresholdDb, procDelaySeconds, true);
}

bool PacketEngine::ForwardNextHop(std::shared_ptr<Packet> packet,
                                 const std::string& currentNodeId,
                                 double currentTime,
                                 Core::EventScheduler& scheduler,
                                 double defaultDataRateBps,
                                 double sinrDb,
                                 double minSinrThresholdDb,
                                 double procDelaySeconds) {
    if (!packet) {
        return false;
    }

    if (packet->IsExpired(currentTime) || packet->GetTtl() == 0) {
        DropPacket(packet, Metrics::PacketDropReason::TtlExpired, currentTime);
        return false;
    }

    const std::string& destId = packet->GetDestinationNodeId();
    if (currentNodeId == destId) {
        DeliverPacket(packet, currentTime);
        return true;
    }

    Routing::RoutingEngine* router = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        router = m_routingEngineRaw;
    }

    if (!router) {
        return false;
    }

    std::string nextHop = router->ResolveNextHop(currentNodeId, destId);
    if (nextHop.empty()) {
        DropPacket(packet, Metrics::PacketDropReason::NoRoute, currentTime);
        return false;
    }

    double hopDist = 1000.0e3;
    double hopRate = defaultDataRateBps;

    auto linkOpt = router->GetLink(currentNodeId, nextHop);
    if (linkOpt.has_value()) {
        if (!linkOpt->isActive) {
            DropPacket(packet, Metrics::PacketDropReason::NoRoute, currentTime);
            return false;
        }
        hopDist = linkOpt->distanceMeters;
        hopRate = linkOpt->dataRateBps;
    }

    return ScheduleHopTransmission(packet, nextHop, hopRate, hopDist, sinrDb, currentTime,
                                  scheduler, minSinrThresholdDb, procDelaySeconds, true);
}

bool PacketEngine::ScheduleHopTransmission(std::shared_ptr<Packet> packet,
                                          const std::string& nextNodeId,
                                          double dataRateBps,
                                          double distanceMeters,
                                          double sinrDb,
                                          double currentTime,
                                          Core::EventScheduler& scheduler,
                                          double minSinrThresholdDb,
                                          double procDelaySeconds,
                                          bool autoForwardMultiHop) {
    auto res = TransmitHop(packet, nextNodeId, dataRateBps, distanceMeters,
                           sinrDb, currentTime, minSinrThresholdDb, procDelaySeconds);
    if (!res.success) {
        return false;
    }

    double arrivalTime = res.arrivalTime;
    if (nextNodeId == packet->GetDestinationNodeId()) {
        // Destination node reached (e.g. gateway)
        scheduler.ScheduleEvent(arrivalTime, 5, Core::EventType::PacketDelivered,
                                [this, packet, arrivalTime]() {
                                    DeliverPacket(packet, arrivalTime);
                                });
    } else {
        // Intermediate hop (e.g. inter-satellite link) -> trigger routing at arrival
        scheduler.ScheduleEvent(arrivalTime, 5, Core::EventType::RoutePacket,
                                [this, packet, nextNodeId, arrivalTime, &scheduler, dataRateBps, sinrDb, minSinrThresholdDb, procDelaySeconds, autoForwardMultiHop]() {
                                    if (autoForwardMultiHop) {
                                        ForwardNextHop(packet, nextNodeId, arrivalTime, scheduler,
                                                       dataRateBps, sinrDb, minSinrThresholdDb, procDelaySeconds);
                                    }
                                });
    }

    return true;
}

uint64_t PacketEngine::GetTotalPacketsSent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalSent;
}

uint64_t PacketEngine::GetTotalPacketsDelivered() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalDelivered;
}

uint64_t PacketEngine::GetTotalPacketsDropped() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalDropped;
}

} // namespace NtnSim::Packet
