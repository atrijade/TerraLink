#include "ntn-sim/coverage/coverage_info.h"

namespace NtnSim::Coverage {

CoverageInfo::CoverageInfo() {}

CoverageInfo::~CoverageInfo() {}

bool CoverageInfo::IsVisible() const {
    return m_isVisible;
}

void CoverageInfo::SetVisible(bool val) {
    m_isVisible = val;
}

double CoverageInfo::GetDistanceMeters() const {
    return m_distanceMeters;
}

void CoverageInfo::SetDistanceMeters(double dist) {
    m_distanceMeters = dist;
}

double CoverageInfo::GetElevationAngleDeg() const {
    return m_elevationAngleDeg;
}

void CoverageInfo::SetElevationAngleDeg(double angle) {
    m_elevationAngleDeg = angle;
}

} // namespace NtnSim::Coverage
