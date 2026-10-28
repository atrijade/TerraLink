#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/logger/logger.h"

namespace NtnSim::Gateway {

GatewayEngine::GatewayEngine() {}

GatewayEngine::GatewayEngine(Metrics::MetricsEngine* metricsEngine, Routing::RoutingEngine* routingEngine)
    : m_routingEngine(routingEngine), m_metricsEngine(metricsEngine) {}

GatewayEngine::~GatewayEngine() {}

// =========================================================================
// Fleet Management
// =========================================================================

void GatewayEngine::AddGateway(std::shared_ptr<Gateway> gateway) {
    if (!gateway) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gateways[gateway->GetId()] = gateway;
    m_gatewaysRaw[gateway->GetId()] = gateway.get();

    if (m_packetEngine) {
        m_packetEngine->RegisterGateway(gateway.get());
    }
}

void GatewayEngine::AddGateway(Gateway* gateway) {
    if (!gateway) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gatewaysRaw[gateway->GetId()] = gateway;

    if (m_packetEngine) {
        m_packetEngine->RegisterGateway(gateway);
    }
}

bool GatewayEngine::RemoveGateway(const std::string& gatewayId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_packetEngine) {
        m_packetEngine->UnregisterGateway(gatewayId);
    }
    m_gatewaysRaw.erase(gatewayId);
    return m_gateways.erase(gatewayId) > 0;
}

std::shared_ptr<Gateway> GatewayEngine::GetGateway(const std::string& gatewayId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_gateways.find(gatewayId);
    if (it != m_gateways.end()) {
        return it->second;
    }
    return nullptr;
}

Gateway* GatewayEngine::GetGatewayRaw(const std::string& gatewayId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_gatewaysRaw.find(gatewayId);
    if (it != m_gatewaysRaw.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Gateway>> GatewayEngine::GetAllGateways() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Gateway>> result;
    result.reserve(m_gateways.size());
    for (const auto& [id, gw] : m_gateways) {
        result.push_back(gw);
    }
    return result;
}

size_t GatewayEngine::GetGatewayCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_gatewaysRaw.size();
}

void GatewayEngine::ClearGateways() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gateways.clear();
    m_gatewaysRaw.clear();
}

// =========================================================================
// Engine Dependency Injection
// =========================================================================

void GatewayEngine::SetOrbitEngine(Orbit::OrbitEngine* orbitEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_orbitEngine = orbitEngine;
}

void GatewayEngine::SetCoverageEngine(Coverage::CoverageEngine* coverageEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coverageEngine = coverageEngine;
}

void GatewayEngine::SetSignalEngine(Signal::SignalEngine* signalEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signalEngine = signalEngine;
}

void GatewayEngine::SetRoutingEngine(Routing::RoutingEngine* routingEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routingEngine = routingEngine;
}

void GatewayEngine::SetPacketEngine(Packet::PacketEngine* packetEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packetEngine = packetEngine;
    if (m_packetEngine) {
        for (const auto& [id, gw] : m_gatewaysRaw) {
            m_packetEngine->RegisterGateway(gw);
        }
    }
}

void GatewayEngine::SetMetricsEngine(Metrics::MetricsEngine* metricsEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsEngine = metricsEngine;
}

Orbit::OrbitEngine* GatewayEngine::GetOrbitEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_orbitEngine;
}

Coverage::CoverageEngine* GatewayEngine::GetCoverageEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coverageEngine;
}

Signal::SignalEngine* GatewayEngine::GetSignalEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_signalEngine;
}

Routing::RoutingEngine* GatewayEngine::GetRoutingEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_routingEngine;
}

Packet::PacketEngine* GatewayEngine::GetPacketEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_packetEngine;
}

Metrics::MetricsEngine* GatewayEngine::GetMetricsEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metricsEngine;
}

// =========================================================================
// Step 3: Feeder Link Handover & Tracking Loop
// =========================================================================

void GatewayEngine::UpdateFeederLinks(double currentTime) {
    std::vector<std::shared_ptr<Orbit::Satellite>> satellites;
    Routing::RoutingEngine* router = nullptr;
    Signal::SignalEngine* signalEngine = nullptr;
    Metrics::MetricsEngine* metrics = nullptr;
    std::vector<Gateway*> gateways;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_orbitEngine) {
            satellites = m_orbitEngine->GetAllSatellites();
        }
        router = m_routingEngine;
        signalEngine = m_signalEngine;
        metrics = m_metricsEngine;

        gateways.reserve(m_gatewaysRaw.size());
        for (const auto& [id, gw] : m_gatewaysRaw) {
            if (gw) {
                gateways.push_back(gw);
            }
        }
    }

    if (satellites.empty() || gateways.empty()) {
        return;
    }

    for (auto* gw : gateways) {
        // Evaluate feeder handover and update RoutingEngine
        gw->EvaluateFeederHandover(satellites, router, currentTime, metrics);

        // Compute and update SignalEngine active connection if connected
        std::string satId = gw->GetConnectedSatelliteId();
        if (!satId.empty() && signalEngine) {
            for (const auto& sat : satellites) {
                if (sat && sat->GetId() == satId) {
                    auto sig = gw->CalculateFeederSignal(*sat, signalEngine);
                    signalEngine->UpdateConnection(gw->GetId(), satId, sig);
                    break;
                }
            }
        }

        // Record telemetry into MetricsEngine
        if (metrics) {
            gw->RecordMetrics(metrics, currentTime);
        }
    }
}

// =========================================================================
// Step 4: Traffic Injection & Telemetry
// =========================================================================

std::shared_ptr<Packet::Packet> GatewayEngine::InjectUplinkPacket(
    const std::string& gatewayId,
    uint64_t packetId,
    uint32_t sizeBytes,
    double currentTime,
    const std::string& destinationId,
    Core::EventScheduler* scheduler) {

    Gateway* gw = GetGatewayRaw(gatewayId);
    Packet::PacketEngine* pe = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        pe = m_packetEngine;
    }

    if (!gw || !pe) {
        return nullptr;
    }

    return gw->InjectUplinkPacket(*pe, packetId, sizeBytes, currentTime, destinationId, scheduler);
}

void GatewayEngine::RecordMetrics(double currentTime) {
    Metrics::MetricsEngine* metrics = nullptr;
    std::vector<Gateway*> gateways;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        metrics = m_metricsEngine;
        for (const auto& [id, gw] : m_gatewaysRaw) {
            if (gw) {
                gateways.push_back(gw);
            }
        }
    }

    if (!metrics) {
        return;
    }

    for (auto* gw : gateways) {
        gw->RecordMetrics(metrics, currentTime);
    }
}

} // namespace NtnSim::Gateway
