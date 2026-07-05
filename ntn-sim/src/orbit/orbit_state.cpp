#include "ntn-sim/orbit/orbit_state.h"

namespace NtnSim::Orbit {

OrbitState::OrbitState() {}

OrbitState::~OrbitState() {}

void OrbitState::Propagate(double epochSeconds) {
    (void)epochSeconds;
    // Skeleton implementation
}

Utils::Position3D OrbitState::GetEcefPosition() const {
    return m_ecefPosition;
}

void OrbitState::SetEcefPosition(const Utils::Position3D& pos) {
    m_ecefPosition = pos;
}

double OrbitState::GetVelocityKms() const {
    return m_velocityKms;
}

void OrbitState::SetVelocityKms(double vel) {
    m_velocityKms = vel;
}

double OrbitState::GetAltitudeKm() const {
    return m_altitudeKm;
}

void OrbitState::SetAltitudeKm(double alt) {
    m_altitudeKm = alt;
}

} // namespace NtnSim::Orbit
