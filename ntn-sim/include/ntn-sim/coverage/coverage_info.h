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

private:
    bool m_isVisible{false};
    double m_distanceMeters{0.0};
    double m_elevationAngleDeg{0.0};
};

} // namespace NtnSim::Coverage
