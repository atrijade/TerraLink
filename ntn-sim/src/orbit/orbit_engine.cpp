#include "ntn-sim/orbit/orbit_engine.h"
#include <algorithm>

namespace NtnSim::Orbit {

OrbitEngine::OrbitEngine() {}

OrbitEngine::~OrbitEngine() {}

void OrbitEngine::UpdatePositions(double simTimeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sat : m_satellites) {
        OrbitState state = sat->GetOrbitState();
        state.Propagate(simTimeSeconds);
        sat->SetOrbitState(state);
    }
}

void OrbitEngine::AddSatellite(std::shared_ptr<Satellite> sat) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_satellites.push_back(sat);
}

std::shared_ptr<Satellite> OrbitEngine::GetSatellite(const std::string& satId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_satellites.begin(), m_satellites.end(), [&satId](const auto& sat) {
        return sat->GetId() == satId;
    });
    if (it != m_satellites.end()) {
        return *it;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Satellite>> OrbitEngine::GetAllSatellites() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_satellites;
}

bool OrbitEngine::LoadTleData(const std::string& tleFilePath) {
    (void)tleFilePath;
    // Skeleton implementation
    return true;
}

} // namespace NtnSim::Orbit
