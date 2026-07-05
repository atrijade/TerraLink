#pragma once

#include "ntn-sim/orbit/satellite.h"
#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace NtnSim::Orbit {

/**
 * @brief Manages the positions, trajectories, and propagation of all satellites.
 */
class OrbitEngine {
public:
    OrbitEngine();
    ~OrbitEngine();

    /**
     * @brief Propagates all satellite positions to the target simulation time.
     * @param simTimeSeconds Current simulation time.
     */
    void UpdatePositions(double simTimeSeconds);

    /**
     * @brief Registers a satellite in the engine.
     */
    void AddSatellite(std::shared_ptr<Satellite> sat);

    /**
     * @brief Gets a satellite pointer by ID.
     */
    std::shared_ptr<Satellite> GetSatellite(const std::string& satId) const;

    /**
     * @brief Returns list of all tracked satellites.
     */
    std::vector<std::shared_ptr<Satellite>> GetAllSatellites() const;

    /**
     * @brief Loads orbital paths from TLE formats (placeholder).
     */
    bool LoadTleData(const std::string& tleFilePath);

private:
    std::vector<std::shared_ptr<Satellite>> m_satellites;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Orbit
