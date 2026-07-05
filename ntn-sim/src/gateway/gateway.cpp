#include "ntn-sim/gateway/gateway.h"

namespace NtnSim::Gateway {

Gateway::Gateway(const std::string& id, const Utils::GeodeticPosition& position)
    : m_id(id), m_position(position) {}

Gateway::~Gateway() {}

std::string Gateway::GetId() const {
    return m_id;
}

Utils::GeodeticPosition Gateway::GetPosition() const {
    return m_position;
}

bool Gateway::ReceivePacket(uint64_t packetId) {
    (void)packetId;
    // Skeleton implementation
    return true;
}

} // namespace NtnSim::Gateway
