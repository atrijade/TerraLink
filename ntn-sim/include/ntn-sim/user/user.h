#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>

namespace NtnSim::User {

/**
 * @brief Model representing a User Equipment (UE) terminal.
 */
class User {
public:
    /**
     * @brief Constructs a User model.
     * @param id Unique user ID.
     * @param initialPos Initial location on Earth.
     */
    User(const std::string& id, const Utils::GeodeticPosition& initialPos);
    ~User();

    std::string GetId() const;
    Utils::GeodeticPosition GetPosition() const;
    void SetPosition(const Utils::GeodeticPosition& pos);

    std::string GetServingSatelliteId() const;
    void SetServingSatelliteId(const std::string& satId);

    double GetTrafficRatePps() const;
    void SetTrafficRatePps(double rate);

private:
    std::string m_id;
    Utils::GeodeticPosition m_position;
    std::string m_servingSatelliteId;
    double m_trafficRatePps{10.0};
};

} // namespace NtnSim::User
