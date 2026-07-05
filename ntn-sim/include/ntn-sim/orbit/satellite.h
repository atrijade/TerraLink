#pragma once

#include "ntn-sim/orbit/orbit_state.h"
#include <string>

namespace NtnSim::Orbit {

/**
 * @brief Model representing a single LEO satellite node.
 */
class Satellite {
public:
    /**
     * @brief Constructs a Satellite.
     * @param id Unique satellite identifier.
     * @param state Initial orbital state.
     */
    Satellite(const std::string& id, const OrbitState& state);
    ~Satellite();

    std::string GetId() const;
    OrbitState GetOrbitState() const;
    void SetOrbitState(const OrbitState& state);

    double GetCoverageRadiusKm() const;
    void SetCoverageRadiusKm(double radius);

private:
    std::string m_id;
    OrbitState m_state;
    double m_coverageRadiusKm{500.0};
};

} // namespace NtnSim::Orbit
