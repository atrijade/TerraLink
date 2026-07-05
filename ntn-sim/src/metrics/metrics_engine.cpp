#include "ntn-sim/metrics/metrics_engine.h"
#include <numeric>

namespace NtnSim::Metrics {

MetricsEngine::MetricsEngine() {}

MetricsEngine::~MetricsEngine() {}

void MetricsEngine::RecordMetric(const std::string& name, double value, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.emplace_back(name, value, timestamp);
}

double MetricsEngine::GetAverageMetric(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double sum = 0.0;
    size_t count = 0;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            sum += record.GetValue();
            count++;
        }
    }
    if (count == 0) {
        return 0.0;
    }
    return sum / count;
}

bool MetricsEngine::ExportToJson(const std::string& filepath) const {
    (void)filepath;
    // Skeleton implementation
    return true;
}

void MetricsEngine::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
}

} // namespace NtnSim::Metrics
