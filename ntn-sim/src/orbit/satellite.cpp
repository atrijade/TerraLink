#include "ntn-sim/orbit/satellite.h"

namespace NtnSim::Orbit {

Satellite::Satellite(const std::string& id, const OrbitState& state)
    : m_id(id), m_state(state) {}

Satellite::~Satellite() {}

std::string Satellite::GetId() const {
    return m_id;
}

OrbitState Satellite::GetOrbitState() const {
    return m_state;
}

void Satellite::SetOrbitState(const OrbitState& state) {
    m_state = state;
}

double Satellite::GetCoverageRadiusKm() const {
    return m_coverageRadiusKm;
}

void Satellite::SetCoverageRadiusKm(double radius) {
    m_coverageRadiusKm = radius;
}

} // namespace NtnSim::Orbit
