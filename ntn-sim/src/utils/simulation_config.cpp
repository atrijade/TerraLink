#include "ntn-sim/utils/simulation_config.h"

namespace NtnSim::Utils {

SimulationConfig::SimulationConfig() {}

SimulationConfig::~SimulationConfig() {}

double SimulationConfig::GetDurationSeconds() const {
    return m_durationSeconds;
}

void SimulationConfig::SetDurationSeconds(double val) {
    m_durationSeconds = val;
}

double SimulationConfig::GetTimeStepSeconds() const {
    return m_timeStepSeconds;
}

void SimulationConfig::SetTimeStepSeconds(double val) {
    m_timeStepSeconds = val;
}

unsigned int SimulationConfig::GetSeed() const {
    return m_seed;
}

void SimulationConfig::SetSeed(unsigned int val) {
    m_seed = val;
}

const std::vector<SatelliteConfig>& SimulationConfig::GetSatellites() const {
    return m_satellites;
}

void SimulationConfig::AddSatellite(const SatelliteConfig& sat) {
    m_satellites.push_back(sat);
}

const std::vector<GatewayConfig>& SimulationConfig::GetGateways() const {
    return m_gateways;
}

void SimulationConfig::AddGateway(const GatewayConfig& gw) {
    m_gateways.push_back(gw);
}

} // namespace NtnSim::Utils
