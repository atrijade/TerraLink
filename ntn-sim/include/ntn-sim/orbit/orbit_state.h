#pragma once

#include "ntn-sim/utils/math_utils.h"

namespace NtnSim::Orbit {

/**
 * @brief Represents the dynamic orbital parameters and physical position of a satellite.
 */
class OrbitState {
public:
    OrbitState();
    ~OrbitState();

    /**
     * @brief Computes positions based on Keplerian parameters at a given time.
     * @param epochSeconds Time in seconds from simulation epoch.
     */
    void Propagate(double epochSeconds);

    Utils::Position3D GetEcefPosition() const;
    void SetEcefPosition(const Utils::Position3D& pos);

    double GetVelocityKms() const;
    void SetVelocityKms(double vel);

    double GetAltitudeKm() const;
    void SetAltitudeKm(double alt);

private:
    Utils::Position3D m_ecefPosition;
    double m_velocityKms{7.5};
    double m_altitudeKm{600.0};
};

} // namespace NtnSim::Orbit
