#include "ntn-sim/signal/signal_info.h"

namespace NtnSim::Signal {

SignalInfo::SignalInfo() {}

SignalInfo::~SignalInfo() {}

double SignalInfo::GetPathLossDb() const {
    return m_pathLossDb;
}

void SignalInfo::SetPathLossDb(double loss) {
    m_pathLossDb = loss;
}

double SignalInfo::GetRsrpDbm() const {
    return m_rsrpDbm;
}

void SignalInfo::SetRsrpDbm(double rsrp) {
    m_rsrpDbm = rsrp;
}

double SignalInfo::GetSinrDb() const {
    return m_sinrDb;
}

void SignalInfo::SetSinrDb(double sinr) {
    m_sinrDb = sinr;
}

double SignalInfo::GetLatencySeconds() const {
    return m_latencySeconds;
}

void SignalInfo::SetLatencySeconds(double latency) {
    m_latencySeconds = latency;
}

double SignalInfo::GetDopplerShiftHz() const {
    return m_dopplerShiftHz;
}

void SignalInfo::SetDopplerShiftHz(double doppler) {
    m_dopplerShiftHz = doppler;
}

} // namespace NtnSim::Signal
