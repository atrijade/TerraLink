#pragma once

#include "ntn-sim/signal/signal_info.h"
#include <string>

namespace NtnSim::Signal {

/**
 * @brief Model representing an active communications link between two network nodes.
 */
class Connection {
public:
    Connection(const std::string& sourceId, const std::string& destinationId);
    ~Connection();

    std::string GetSourceId() const;
    std::string GetDestinationId() const;

    bool IsActive() const;
    void SetActive(bool active);

    SignalInfo GetSignalInfo() const;
    void SetSignalInfo(const SignalInfo& info);

private:
    std::string m_sourceId;
    std::string m_destinationId;
    bool m_isActive{false};
    SignalInfo m_signalInfo;
};

} // namespace NtnSim::Signal
