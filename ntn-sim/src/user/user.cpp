#include "ntn-sim/user/user.h"

namespace NtnSim::User {

User::User(const std::string& id, const Utils::GeodeticPosition& initialPos)
    : m_id(id), m_position(initialPos) {}

User::~User() {}

std::string User::GetId() const {
    return m_id;
}

Utils::GeodeticPosition User::GetPosition() const {
    return m_position;
}

void User::SetPosition(const Utils::GeodeticPosition& pos) {
    m_position = pos;
}

std::string User::GetServingSatelliteId() const {
    return m_servingSatelliteId;
}

void User::SetServingSatelliteId(const std::string& satId) {
    m_servingSatelliteId = satId;
}

double User::GetTrafficRatePps() const {
    return m_trafficRatePps;
}

void User::SetTrafficRatePps(double rate) {
    m_trafficRatePps = rate;
}

} // namespace NtnSim::User
