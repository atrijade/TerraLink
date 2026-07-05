#pragma once

namespace NtnSim::Signal {

/**
 * @brief RF Channel propagation properties.
 */
class SignalInfo {
public:
    SignalInfo();
    ~SignalInfo();

    double GetPathLossDb() const;
    void SetPathLossDb(double loss);

    double GetRsrpDbm() const;
    void SetRsrpDbm(double rsrp);

    double GetSinrDb() const;
    void SetSinrDb(double sinr);

    double GetLatencySeconds() const;
    void SetLatencySeconds(double latency);

    double GetDopplerShiftHz() const;
    void SetDopplerShiftHz(double doppler);

private:
    double m_pathLossDb{120.0};
    double m_rsrpDbm{-90.0};
    double m_sinrDb{15.0};
    double m_latencySeconds{0.02}; // e.g. 20 ms
    double m_dopplerShiftHz{1000.0};
};

} // namespace NtnSim::Signal
