#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>
#include <cstdint>

namespace NtnSim::Gateway {

/**
 * @brief Represents an Internet Gateway or satellite Ground Station.
 */
class Gateway {
public:
    /**
     * @brief Constructs a new Gateway component.
     * @param id Unique gateway identifier.
     * @param position Geodetic position on Earth.
     */
    Gateway(const std::string& id, const Utils::GeodeticPosition& position);
    ~Gateway();

    std::string GetId() const;
    Utils::GeodeticPosition GetPosition() const;

    /**
     * @brief Placeholder for receiving a packet downlinked from satellite.
     * @param packetId ID of the received packet.
     * @return True if packet processed successfully.
     */
    bool ReceivePacket(uint64_t packetId);

private:
    std::string m_id;
    Utils::GeodeticPosition m_position;
};

} // namespace NtnSim::Gateway
