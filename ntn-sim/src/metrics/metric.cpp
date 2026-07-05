#include "ntn-sim/metrics/metric.h"

namespace NtnSim::Metrics {

Metric::Metric(const std::string& name, double value, double timestamp)
    : m_name(name), m_value(value), m_timestamp(timestamp) {}

Metric::~Metric() {}

std::string Metric::GetName() const {
    return m_name;
}

double Metric::GetValue() const {
    return m_value;
}

double Metric::GetTimestamp() const {
    return m_timestamp;
}

} // namespace NtnSim::Metrics
