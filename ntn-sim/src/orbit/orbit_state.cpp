#include "ntn-sim/orbit/orbit_state.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace NtnSim::Orbit {

namespace {

inline double DegToRad(double deg) {
    return deg * std::numbers::pi / 180.0;
}

inline double RadToDeg(double rad) {
    return rad * 180.0 / std::numbers::pi;
}

inline double NormalizeRadians(double rad) {
    rad = std::fmod(rad, 2.0 * std::numbers::pi);
    if (rad < 0.0) {
        rad += 2.0 * std::numbers::pi;
    }
    return rad;
}

inline double NormalizeDegrees(double deg) {
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) {
        deg += 360.0;
    }
    return deg;
}

} // anonymous namespace

OrbitState::OrbitState() {
    m_semiMajorAxisKm = EARTH_RADIUS_KM + m_altitudeKm;
    m_velocityKms = std::sqrt(EARTH_MU_KM3_S2 / m_semiMajorAxisKm);
}

OrbitState::OrbitState(double altitudeKm, double inclinationDeg, double raanDeg,
                       double argPerigeeDeg, double meanAnomalyEpochDeg,
                       double eccentricity)
    : m_altitudeKm(altitudeKm),
      m_semiMajorAxisKm(EARTH_RADIUS_KM + altitudeKm),
      m_eccentricity(eccentricity),
      m_inclinationDeg(inclinationDeg),
      m_raanDeg(raanDeg),
      m_argPerigeeDeg(argPerigeeDeg),
      m_meanAnomalyEpochDeg(meanAnomalyEpochDeg) {
    m_velocityKms = std::sqrt(EARTH_MU_KM3_S2 / m_semiMajorAxisKm);
    Propagate(0.0);
}

OrbitState::~OrbitState() {}

void OrbitState::Propagate(double epochSeconds) {
    if (m_semiMajorAxisKm <= 0.0) {
        return;
    }

    // Mean motion: n = sqrt(mu / a^3) in rad/s
    double a_cubed = m_semiMajorAxisKm * m_semiMajorAxisKm * m_semiMajorAxisKm;
    double meanMotionRadS = std::sqrt(EARTH_MU_KM3_S2 / a_cubed);

    // Delta time from initial epoch
    double dt = epochSeconds - m_epochSeconds;

    // Time evolution of mean anomaly: M(t) = M0 + n * dt
    double M_rad = NormalizeRadians(DegToRad(m_meanAnomalyEpochDeg) + meanMotionRadS * dt);
    m_currentMeanAnomalyDeg = NormalizeDegrees(RadToDeg(M_rad));

    // Solve Kepler's equation: M = E - e * sin(E) for Eccentric Anomaly E
    double E_rad = M_rad;
    if (m_eccentricity > 1e-7) {
        for (int iter = 0; iter < 100; ++iter) {
            double f = E_rad - m_eccentricity * std::sin(E_rad) - M_rad;
            if (std::abs(f) < 1e-12) {
                break;
            }
            double f_prime = 1.0 - m_eccentricity * std::cos(E_rad);
            E_rad -= f / f_prime;
        }
    }

    // True anomaly nu from Eccentric Anomaly E
    double nu_rad = E_rad;
    if (m_eccentricity > 1e-7) {
        double sin_nu = (std::sqrt(1.0 - m_eccentricity * m_eccentricity) * std::sin(E_rad)) /
                        (1.0 - m_eccentricity * std::cos(E_rad));
        double cos_nu = (std::cos(E_rad) - m_eccentricity) /
                        (1.0 - m_eccentricity * std::cos(E_rad));
        nu_rad = NormalizeRadians(std::atan2(sin_nu, cos_nu));
    }
    m_currentTrueAnomalyDeg = NormalizeDegrees(RadToDeg(nu_rad));

    // Radial distance r (km and meters)
    double r_km = m_semiMajorAxisKm * (1.0 - m_eccentricity * std::cos(E_rad));
    double r_m = r_km * 1000.0;

    // Coordinates in the orbital plane (perifocal frame P-Q) in meters
    double x_orb = r_m * std::cos(nu_rad);
    double y_orb = r_m * std::sin(nu_rad);
    double z_orb = 0.0;
    m_orbitalPlanePosition = Utils::Position3D{x_orb, y_orb, z_orb};

    // Orbital speed (vis-viva equation: v = sqrt(mu * (2/r - 1/a)))
    m_velocityKms = std::sqrt(EARTH_MU_KM3_S2 * (2.0 / r_km - 1.0 / m_semiMajorAxisKm));

    // Transformation from Orbital Plane (Perifocal) to Earth-Centered Inertial (ECI)
    double raan_rad = DegToRad(m_raanDeg);
    double inc_rad = DegToRad(m_inclinationDeg);
    double omega_rad = DegToRad(m_argPerigeeDeg);

    double cos_raan = std::cos(raan_rad);
    double sin_raan = std::sin(raan_rad);
    double cos_inc = std::cos(inc_rad);
    double sin_inc = std::sin(inc_rad);
    double cos_omega = std::cos(omega_rad);
    double sin_omega = std::sin(omega_rad);

    double p11 = cos_raan * cos_omega - sin_raan * sin_omega * cos_inc;
    double p12 = -cos_raan * sin_omega - sin_raan * cos_omega * cos_inc;

    double p21 = sin_raan * cos_omega + cos_raan * sin_omega * cos_inc;
    double p22 = -sin_raan * sin_omega + cos_raan * cos_omega * cos_inc;

    double p31 = sin_omega * sin_inc;
    double p32 = cos_omega * sin_inc;

    double x_eci = p11 * x_orb + p12 * y_orb;
    double y_eci = p21 * x_orb + p22 * y_orb;
    double z_eci = p31 * x_orb + p32 * y_orb;
    m_eciPosition = Utils::Position3D{x_eci, y_eci, z_eci};

    // Transformation from ECI to Earth-Centered Earth-Fixed (ECEF) via Earth rotation
    double theta_gmst = NormalizeRadians(EARTH_ROTATION_RATE_RAD_S * epochSeconds);
    double cos_theta = std::cos(theta_gmst);
    double sin_theta = std::sin(theta_gmst);

    double x_ecef = cos_theta * x_eci + sin_theta * y_eci;
    double y_ecef = -sin_theta * x_eci + cos_theta * y_eci;
    double z_ecef = z_eci;
    m_ecefPosition = Utils::Position3D{x_ecef, y_ecef, z_ecef};

    // Velocity vector in the orbital plane (perifocal frame P-Q), km/s
    // Using vis-viva angular momentum: h = sqrt(mu * p), p = a*(1-e^2)
    double p_sl   = m_semiMajorAxisKm * (1.0 - m_eccentricity * m_eccentricity);
    double h_ang  = std::sqrt(EARTH_MU_KM3_S2 * p_sl);
    double hp     = h_ang / p_sl;
    double vx_orb = hp * (-std::sin(nu_rad));
    double vy_orb = hp * (m_eccentricity + std::cos(nu_rad));

    // Rotate perifocal velocity into ECI using the same rotation matrix (p11..p32)
    double vx_eci = p11 * vx_orb + p12 * vy_orb;
    double vy_eci = p21 * vx_orb + p22 * vy_orb;
    double vz_eci = p31 * vx_orb + p32 * vy_orb;

    // Rotate ECI velocity into ECEF and apply Earth-rotation Coriolis correction
    // v_ECEF = R_z(theta) * v_ECI + omega_E x r_ECEF
    double vx_ecef =  cos_theta * vx_eci + sin_theta * vy_eci
                    + EARTH_ROTATION_RATE_RAD_S * y_ecef;
    double vy_ecef = -sin_theta * vx_eci + cos_theta * vy_eci
                    - EARTH_ROTATION_RATE_RAD_S * x_ecef;
    double vz_ecef = vz_eci;
    m_ecefVelocity = Utils::Position3D{vx_ecef, vy_ecef, vz_ecef};
}

Utils::Position3D OrbitState::GetEcefPosition() const {
    return m_ecefPosition;
}

void OrbitState::SetEcefPosition(const Utils::Position3D& pos) {
    m_ecefPosition = pos;
}

Utils::Position3D OrbitState::GetEciPosition() const {
    return m_eciPosition;
}

void OrbitState::SetEciPosition(const Utils::Position3D& pos) {
    m_eciPosition = pos;
}

Utils::Position3D OrbitState::GetOrbitalPlanePosition() const {
    return m_orbitalPlanePosition;
}

double OrbitState::GetVelocityKms() const {
    return m_velocityKms;
}

void OrbitState::SetVelocityKms(double vel) {
    m_velocityKms = vel;
}

Utils::Position3D OrbitState::GetEcefVelocity() const {
    return m_ecefVelocity;
}

void OrbitState::SetEcefVelocity(const Utils::Position3D& vel) {
    m_ecefVelocity = vel;
}

double OrbitState::GetAltitudeKm() const {
    return m_altitudeKm;
}

void OrbitState::SetAltitudeKm(double alt) {
    m_altitudeKm = alt;
    m_semiMajorAxisKm = EARTH_RADIUS_KM + alt;
}

double OrbitState::GetSemiMajorAxisKm() const {
    return m_semiMajorAxisKm;
}

void OrbitState::SetSemiMajorAxisKm(double aKm) {
    m_semiMajorAxisKm = aKm;
    m_altitudeKm = aKm - EARTH_RADIUS_KM;
}

double OrbitState::GetEccentricity() const {
    return m_eccentricity;
}

void OrbitState::SetEccentricity(double ecc) {
    m_eccentricity = ecc;
}

double OrbitState::GetInclinationDeg() const {
    return m_inclinationDeg;
}

void OrbitState::SetInclinationDeg(double incDeg) {
    m_inclinationDeg = incDeg;
}

double OrbitState::GetRaanDeg() const {
    return m_raanDeg;
}

void OrbitState::SetRaanDeg(double raanDeg) {
    m_raanDeg = raanDeg;
}

double OrbitState::GetArgPerigeeDeg() const {
    return m_argPerigeeDeg;
}

void OrbitState::SetArgPerigeeDeg(double argPerigeeDeg) {
    m_argPerigeeDeg = argPerigeeDeg;
}

double OrbitState::GetMeanAnomalyEpochDeg() const {
    return m_meanAnomalyEpochDeg;
}

void OrbitState::SetMeanAnomalyEpochDeg(double maDeg) {
    m_meanAnomalyEpochDeg = maDeg;
}

double OrbitState::GetCurrentMeanAnomalyDeg() const {
    return m_currentMeanAnomalyDeg;
}

double OrbitState::GetCurrentTrueAnomalyDeg() const {
    return m_currentTrueAnomalyDeg;
}

double OrbitState::GetOrbitalPeriodSeconds() const {
    if (m_semiMajorAxisKm <= 0.0) {
        return 0.0;
    }
    double a_cubed = m_semiMajorAxisKm * m_semiMajorAxisKm * m_semiMajorAxisKm;
    return 2.0 * std::numbers::pi * std::sqrt(a_cubed / EARTH_MU_KM3_S2);
}

double OrbitState::GetMeanMotionRadS() const {
    if (m_semiMajorAxisKm <= 0.0) {
        return 0.0;
    }
    double a_cubed = m_semiMajorAxisKm * m_semiMajorAxisKm * m_semiMajorAxisKm;
    return std::sqrt(EARTH_MU_KM3_S2 / a_cubed);
}

} // namespace NtnSim::Orbit
