#pragma once

#include "ntn-sim/metrics/metric.h"
#include <vector>
#include <mutex>
#include <string>

namespace NtnSim::Metrics {

/**
 * @brief Aggregates, stores, and handles exports of KPIs and execution metrics.
 */
class MetricsEngine {
public:
    MetricsEngine();
    ~MetricsEngine();

    /**
     * @brief Logs a metric value into telemetry buffers.
     */
    void RecordMetric(const std::string& name, double value, double timestamp);

    /**
     * @brief Computes mean value of logged samples for a metric name.
     */
    double GetAverageMetric(const std::string& name) const;

    /**
     * @brief Exports all logged telemetry records to a target JSON file.
     */
    bool ExportToJson(const std::string& filepath) const;

    /**
     * @brief Clears current records.
     */
    void Reset();

private:
    std::vector<Metric> m_records;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Metrics
