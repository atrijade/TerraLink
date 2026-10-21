#pragma once

#include "ntn-sim/metrics/metric.h"
#include "ntn-sim/metrics/kpi_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace NtnSim::Metrics {

/**
 * @brief Lightweight telemetry sample for high-throughput buffering.
 */
struct MetricSample {
    double value{0.0};
    double timestamp{0.0};
};

/**
 * @brief Metric time-series tracking samples with O(1) running statistics.
 */
struct MetricSeries {
    std::vector<MetricSample> samples;
    double runningSum{0.0};
    double runningMin{std::numeric_limits<double>::infinity()};
    double runningMax{-std::numeric_limits<double>::infinity()};
};

/**
 * @brief Aggregates, analyzes, and calculates statistical metrics and 3GPP NTN KPIs.
 *        Optimized with O(1) indexed storage and shared_mutex for high event throughput.
 */
class MetricsEngine {
public:
    MetricsEngine();
    ~MetricsEngine();

    // -----------------------------------------------------------------------
    // High-Throughput Generic Telemetry (Phase 1 & Phase 4)
    // -----------------------------------------------------------------------

    /**
     * @brief Logs a metric sample into indexed telemetry buffers. O(1) execution.
     */
    void RecordMetric(const std::string& name, double value, double timestamp);

    /**
     * @brief Returns total number of logged samples for a given metric. O(1) lookup.
     */
    size_t GetCount(const std::string& name) const;

    /**
     * @brief Computes arithmetic mean of logged samples. O(1) lookup.
     */
    double GetAverageMetric(const std::string& name) const;

    /**
     * @brief Computes minimum sample value recorded. O(1) lookup.
     */
    double GetMin(const std::string& name) const;

    /**
     * @brief Computes maximum sample value recorded. O(1) lookup.
     */
    double GetMax(const std::string& name) const;

    /**
     * @brief Computes sample variance for a metric name.
     */
    double GetVariance(const std::string& name) const;

    /**
     * @brief Computes standard deviation for a metric name.
     */
    double GetStandardDeviation(const std::string& name) const;

    /**
     * @brief Computes the P-th percentile (0.0 to 100.0) of a metric using linear interpolation.
     */
    double GetPercentile(const std::string& name, double percentile) const;

    /**
     * @brief Filters samples of a metric within a time window [tStart, tEnd].
     */
    std::vector<Metric> GetSamplesInTimeWindow(const std::string& name, double tStart, double tEnd) const;

    /**
     * @brief Computes average metric value within a specific time window [tStart, tEnd].
     */
    double GetAverageMetricInWindow(const std::string& name, double tStart, double tEnd) const;

    // -----------------------------------------------------------------------
    // Domain-Specific 3GPP NTN KPI Aggregators (Phase 2)
    // -----------------------------------------------------------------------

    /**
     * @brief Records a satellite handover execution attempt.
     */
    void RecordHandover(const std::string& userId, const std::string& fromSatId,
                        const std::string& toSatId, double timestamp,
                        bool success, double interruptionTimeSeconds = 0.0);

    /**
     * @brief Calculates aggregated Handover KPIs including ping-pong rate.
     * @param pingPongThresholdSeconds Maximum interval to consider a return switch as ping-pong (default 5.0s).
     */
    HandoverKPIs CalculateHandoverKPIs(double pingPongThresholdSeconds = 5.0) const;

    /**
     * @brief Records a radio link channel observation.
     */
    void RecordRadioSample(const std::string& userId, const std::string& satId,
                           double rsrpDbm, double sinrDb, double dopplerShiftHz,
                           bool inCoverage, double timestamp);

    /**
     * @brief Calculates aggregated Radio & Channel quality KPIs.
     * @param sinrOutageThresholdDb SINR threshold below which channel is in outage (default -3.0 dB).
     */
    RadioKPIs CalculateRadioKPIs(double sinrOutageThresholdDb = -3.0) const;

    /**
     * @brief Records a packet transmission event.
     */
    void RecordPacketSent(uint64_t packetId, size_t sizeBytes, double timestamp);

    /**
     * @brief Records a successful packet reception event.
     */
    void RecordPacketReceived(uint64_t packetId, double latencySeconds, double timestamp);

    /**
     * @brief Records a packet drop event with a categorized drop reason.
     */
    void RecordPacketDropped(uint64_t packetId, PacketDropReason reason, double timestamp);

    /**
     * @brief Calculates aggregated Traffic, Latency, and Throughput KPIs.
     */
    TrafficKPIs CalculateTrafficKPIs() const;

    // -----------------------------------------------------------------------
    // State Management & Export
    // -----------------------------------------------------------------------

    /**
     * @brief Returns a copy of all generic logged telemetry records.
     */
    std::vector<Metric> GetAllRecords() const;

    /**
     * @brief Exports all logged telemetry records to a target JSON file.
     */
    bool ExportToJson(const std::string& filepath) const;

    /**
     * @brief Clears all current records and KPI telemetry buffers.
     */
    void Reset();

private:
    // O(1) indexed time-series storage with running statistics (Phase 4)
    std::unordered_map<std::string, MetricSeries> m_indexedMetrics;

    // Sequential log preserved for global replay
    std::vector<Metric> m_records;

    // Phase 2 dedicated KPI storage
    std::vector<HandoverRecord> m_handoverRecords;
    std::vector<RadioSample> m_radioSamples;
    std::vector<PacketSentRecord> m_sentPackets;
    std::vector<PacketReceivedRecord> m_receivedPackets;
    std::vector<PacketDropRecord> m_droppedPackets;

    // Read-Write shared mutex for high-concurrency event loops
    mutable std::shared_mutex m_rwMutex;
};

} // namespace NtnSim::Metrics
