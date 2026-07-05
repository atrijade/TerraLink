#include "ntn-sim/signal/connection.h"

namespace NtnSim::Signal {

Connection::Connection(const std::string& sourceId, const std::string& destinationId)
    : m_sourceId(sourceId), m_destinationId(destinationId) {}

Connection::~Connection() {}

std::string Connection::GetSourceId() const {
    return m_sourceId;
}

std::string Connection::GetDestinationId() const {
    return m_destinationId;
}

bool Connection::IsActive() const {
    return m_isActive;
}

void Connection::SetActive(bool active) {
    m_isActive = active;
}

SignalInfo Connection::GetSignalInfo() const {
    return m_signalInfo;
}

void Connection::SetSignalInfo(const SignalInfo& info) {
    m_signalInfo = info;
}

} // namespace NtnSim::Signal
