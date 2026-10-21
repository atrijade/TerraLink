#include "ntn-sim/orbit/orbit_engine.h"
#include <algorithm>
#include <cmath>

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

int OrbitEngine::GenerateWalkerConstellation(const WalkerConfig& config) {
    if (config.planes <= 0 || config.satsPerPlane <= 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_satellites.clear();

    int totalSats = config.planes * config.satsPerPlane;
    m_satellites.reserve(totalSats);

    double raanSpread = config.isWalkerStar ? 180.0 : 360.0;
    double deltaRaan = raanSpread / static_cast<double>(config.planes);
    double deltaMeanAnomaly = 360.0 / static_cast<double>(config.satsPerPlane);
    double deltaPhase = (static_cast<double>(config.phasingParameter) * 360.0) / static_cast<double>(totalSats);

    for (int p = 0; p < config.planes; ++p) {
        double raan = p * deltaRaan;
        for (int s = 0; s < config.satsPerPlane; ++s) {
            double meanAnomaly = std::fmod(s * deltaMeanAnomaly + p * deltaPhase, 360.0);
            if (meanAnomaly < 0.0) {
                meanAnomaly += 360.0;
            }

            std::string satId = "sat-p" + std::to_string(p) + "-s" + std::to_string(s);
            OrbitState state(config.altitudeKm, config.inclinationDeg, raan, 0.0, meanAnomaly, 0.0);

            auto sat = std::make_shared<Satellite>(satId, state);
            sat->SetCoverageRadiusKm(config.coverageRadiusKm);
            m_satellites.push_back(sat);
        }
    }

    return static_cast<int>(m_satellites.size());
}

int OrbitEngine::GenerateWalkerConstellation(int planes, int satsPerPlane, double altitudeKm,
                                            double inclinationDeg, int phasingParameter) {
    WalkerConfig config;
    config.planes = planes;
    config.satsPerPlane = satsPerPlane;
    config.altitudeKm = altitudeKm;
    config.inclinationDeg = inclinationDeg;
    config.phasingParameter = phasingParameter;
    return GenerateWalkerConstellation(config);
}

void OrbitEngine::ClearSatellites() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_satellites.clear();
}

bool OrbitEngine::LoadTleData(const std::string& tleFilePath) {
    (void)tleFilePath;
    // Skeleton implementation
    return true;
}

} // namespace NtnSim::Orbit
