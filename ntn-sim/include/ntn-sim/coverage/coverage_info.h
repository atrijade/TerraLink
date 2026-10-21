#pragma once

namespace NtnSim::Coverage {

/**
 * @brief Spatial relationship data between a user terminal and a satellite.
 */
class CoverageInfo {
public:
    CoverageInfo();
    ~CoverageInfo();

    bool IsVisible() const;
    void SetVisible(bool val);

    double GetDistanceMeters() const;
    void SetDistanceMeters(double dist);

    double GetElevationAngleDeg() const;
    void SetElevationAngleDeg(double angle);

    /**
     * @brief Line-of-sight velocity component of satellite toward user (km/s).
     *        Positive = approaching, Negative = receding.
     *        Used by SignalEngine to compute Doppler frequency shift.
     */
    double GetLosVelocityKms() const;
    void SetLosVelocityKms(double vel);

private:
    bool m_isVisible{false};
    double m_distanceMeters{0.0};
    double m_elevationAngleDeg{0.0};
    double m_losVelocityKms{0.0};
};

} // namespace NtnSim::Coverage
