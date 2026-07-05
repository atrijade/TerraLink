#pragma once

#include <string>

namespace NtnSim::Metrics {

/**
 * @brief Telemetry record representing a single metric sample.
 */
class Metric {
public:
    Metric(const std::string& name, double value, double timestamp);
    ~Metric();

    std::string GetName() const;
    double GetValue() const;
    double GetTimestamp() const;

private:
    std::string m_name;
    double m_value;
    double m_timestamp;
};

} // namespace NtnSim::Metrics
