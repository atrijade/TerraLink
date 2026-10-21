#pragma once

#include "ntn-sim/orbit/satellite.h"
#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace NtnSim::Orbit {

/**
 * @brief Configuration parameters for generating a Walker constellation (i: T/P/F).
 */
struct WalkerConfig {
    int planes{6};                     ///< Number of orbital planes (P)
    int satsPerPlane{12};              ///< Satellites per orbital plane (S = T / P)
    double altitudeKm{600.0};          ///< Altitude in kilometers above Earth surface
    double inclinationDeg{87.5};       ///< Orbital plane inclination in degrees
    int phasingParameter{1};           ///< Walker phasing parameter F (0 <= F < P)
    bool isWalkerStar{false};          ///< false = Walker Delta (360 deg RAAN spread), true = Walker Star (180 deg)
    double coverageRadiusKm{500.0};    ///< Coverage radius of each satellite in km
};

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
     * @brief Generates and registers a Walker constellation of satellites.
     * @param config Walker constellation configuration.
     * @return Number of satellites generated.
     */
    int GenerateWalkerConstellation(const WalkerConfig& config);

    /**
     * @brief Helper overload to generate a Walker constellation.
     */
    int GenerateWalkerConstellation(int planes, int satsPerPlane, double altitudeKm,
                                    double inclinationDeg, int phasingParameter = 1);

    /**
     * @brief Clears all registered satellites.
     */
    void ClearSatellites();

    /**
     * @brief Loads orbital paths from TLE formats (placeholder).
     */
    bool LoadTleData(const std::string& tleFilePath);

private:
    std::vector<std::shared_ptr<Satellite>> m_satellites;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Orbit
