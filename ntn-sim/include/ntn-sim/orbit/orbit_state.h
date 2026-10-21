#pragma once

#include "ntn-sim/utils/math_utils.h"

namespace NtnSim::Orbit {

/**
 * @brief Represents the dynamic orbital parameters and physical position of a satellite.
 */
class OrbitState {
public:
    OrbitState();
    OrbitState(double altitudeKm, double inclinationDeg, double raanDeg = 0.0,
               double argPerigeeDeg = 0.0, double meanAnomalyEpochDeg = 0.0,
               double eccentricity = 0.0);
    ~OrbitState();

    /**
     * @brief Computes positions based on Keplerian parameters at a given time.
     * @param epochSeconds Time in seconds from simulation epoch.
     */
    void Propagate(double epochSeconds);

    Utils::Position3D GetEcefPosition() const;
    void SetEcefPosition(const Utils::Position3D& pos);

    Utils::Position3D GetEciPosition() const;
    void SetEciPosition(const Utils::Position3D& pos);

    Utils::Position3D GetOrbitalPlanePosition() const;

    double GetVelocityKms() const;
    void SetVelocityKms(double vel);

    /**
     * @brief Returns the satellite velocity vector in the ECEF frame (km/s).
     *        Computed automatically during Propagate(). Used for Doppler LoS calculation.
     */
    Utils::Position3D GetEcefVelocity() const;

    double GetAltitudeKm() const;
    void SetAltitudeKm(double alt);

    // Keplerian orbital elements
    double GetSemiMajorAxisKm() const;
    void SetSemiMajorAxisKm(double aKm);

    double GetEccentricity() const;
    void SetEccentricity(double ecc);

    double GetInclinationDeg() const;
    void SetInclinationDeg(double incDeg);

    double GetRaanDeg() const;
    void SetRaanDeg(double raanDeg);

    double GetArgPerigeeDeg() const;
    void SetArgPerigeeDeg(double argPerigeeDeg);

    double GetMeanAnomalyEpochDeg() const;
    void SetMeanAnomalyEpochDeg(double maDeg);

    double GetCurrentMeanAnomalyDeg() const;
    double GetCurrentTrueAnomalyDeg() const;

    double GetOrbitalPeriodSeconds() const;
    double GetMeanMotionRadS() const;

private:
    // Earth physical constants
    static constexpr double EARTH_RADIUS_KM = 6378.137;
    static constexpr double EARTH_MU_KM3_S2 = 398600.4418;           // km^3 / s^2
    static constexpr double EARTH_ROTATION_RATE_RAD_S = 7.292115e-5; // rad/s (WGS-84)

    // Orbital parameters
    double m_altitudeKm{600.0};
    double m_semiMajorAxisKm{6978.137}; // Earth radius + altitude
    double m_eccentricity{0.0};
    double m_inclinationDeg{0.0};
    double m_raanDeg{0.0};
    double m_argPerigeeDeg{0.0};
    double m_meanAnomalyEpochDeg{0.0};
    double m_epochSeconds{0.0};

    // Dynamic state at current simulation time
    double m_currentMeanAnomalyDeg{0.0};
    double m_currentTrueAnomalyDeg{0.0};
    double m_velocityKms{7.5};

    Utils::Position3D m_orbitalPlanePosition{0.0, 0.0, 0.0};
    Utils::Position3D m_eciPosition{0.0, 0.0, 0.0};
    Utils::Position3D m_ecefPosition{0.0, 0.0, 0.0};
    Utils::Position3D m_ecefVelocity{0.0, 0.0, 0.0};  ///< ECEF velocity vector (km/s)
};

} // namespace NtnSim::Orbit
