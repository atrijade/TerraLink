#include "ntn-sim/metrics/metrics_engine.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>

namespace NtnSim::Metrics {

MetricsEngine::MetricsEngine() {}

MetricsEngine::~MetricsEngine() {}

// ---------------------------------------------------------------------------
// Phase 1: Generic Telemetry & Statistical Aggregations
// ---------------------------------------------------------------------------

void MetricsEngine::RecordMetric(const std::string& name, double value, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.emplace_back(name, value, timestamp);
}

size_t MetricsEngine::GetCount(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            count++;
        }
    }
    return count;
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
    return sum / static_cast<double>(count);
}

double MetricsEngine::GetMin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double minVal = std::numeric_limits<double>::infinity();
    bool found = false;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            if (record.GetValue() < minVal) {
                minVal = record.GetValue();
            }
            found = true;
        }
    }
    return found ? minVal : 0.0;
}

double MetricsEngine::GetMax(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double maxVal = -std::numeric_limits<double>::infinity();
    bool found = false;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            if (record.GetValue() > maxVal) {
                maxVal = record.GetValue();
            }
            found = true;
        }
    }
    return found ? maxVal : 0.0;
}

double MetricsEngine::GetVariance(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<double> values;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            values.push_back(record.GetValue());
        }
    }
    if (values.size() < 2) {
        return 0.0;
    }

    double mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    double sumSqDiff = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sumSqDiff += diff * diff;
    }
    return sumSqDiff / static_cast<double>(values.size() - 1); // Sample variance
}

double MetricsEngine::GetStandardDeviation(const std::string& name) const {
    return std::sqrt(GetVariance(name));
}

double MetricsEngine::GetPercentile(const std::string& name, double percentile) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<double> values;
    for (const auto& record : m_records) {
        if (record.GetName() == name) {
            values.push_back(record.GetValue());
        }
    }
    if (values.empty()) {
        return 0.0;
    }
    if (values.size() == 1) {
        return values.front();
    }

    std::sort(values.begin(), values.end());
    double p = std::clamp(percentile, 0.0, 100.0);
    double k = (p / 100.0) * static_cast<double>(values.size() - 1);
    size_t i = static_cast<size_t>(std::floor(k));
    double f = k - static_cast<double>(i);

    if (i + 1 < values.size()) {
        return values[i] + f * (values[i + 1] - values[i]);
    }
    return values.back();
}

std::vector<Metric> MetricsEngine::GetSamplesInTimeWindow(const std::string& name, double tStart, double tEnd) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Metric> result;
    for (const auto& record : m_records) {
        if (record.GetName() == name &&
            record.GetTimestamp() >= tStart &&
            record.GetTimestamp() <= tEnd) {
            result.push_back(record);
        }
    }
    return result;
}

double MetricsEngine::GetAverageMetricInWindow(const std::string& name, double tStart, double tEnd) const {
    auto samples = GetSamplesInTimeWindow(name, tStart, tEnd);
    if (samples.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const auto& s : samples) {
        sum += s.GetValue();
    }
    return sum / static_cast<double>(samples.size());
}

// ---------------------------------------------------------------------------
// Phase 2: Domain-Specific NTN KPI Aggregators
// ---------------------------------------------------------------------------

void MetricsEngine::RecordHandover(const std::string& userId, const std::string& fromSatId,
                                  const std::string& toSatId, double timestamp,
                                  bool success, double interruptionTimeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handoverRecords.push_back(HandoverRecord{
        userId, fromSatId, toSatId, timestamp, success, interruptionTimeSeconds
    });

    // Mirror to generic metrics buffer for time-series access
    m_records.emplace_back("handover_event", success ? 1.0 : 0.0, timestamp);
    if (success && interruptionTimeSeconds > 0.0) {
        m_records.emplace_back("handover_interruption_time", interruptionTimeSeconds, timestamp);
    }
}

HandoverKPIs MetricsEngine::CalculateHandoverKPIs(double pingPongThresholdSeconds) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    HandoverKPIs kpis;
    kpis.totalAttempts = m_handoverRecords.size();
    if (kpis.totalAttempts == 0) {
        return kpis;
    }

    double totalInterruption = 0.0;
    double maxInterruption = 0.0;

    // Track user-specific handover chains for ping-pong detection
    std::unordered_map<std::string, std::vector<const HandoverRecord*>> userHandovers;

    for (const auto& rec : m_handoverRecords) {
        if (rec.success) {
            kpis.totalSuccesses++;
            totalInterruption += rec.interruptionTimeSeconds;
            if (rec.interruptionTimeSeconds > maxInterruption) {
                maxInterruption = rec.interruptionTimeSeconds;
            }
        } else {
            kpis.totalFailures++;
        }
        userHandovers[rec.userId].push_back(&rec);
    }

    kpis.successRatePercent = (static_cast<double>(kpis.totalSuccesses) / static_cast<double>(kpis.totalAttempts)) * 100.0;
    kpis.averageInterruptionTimeSeconds = kpis.totalSuccesses > 0 ? (totalInterruption / static_cast<double>(kpis.totalSuccesses)) : 0.0;
    kpis.maxInterruptionTimeSeconds = maxInterruption;

    // Detect Ping-Pong handovers: User switches A -> B, then B -> A within pingPongThresholdSeconds
    size_t pingPongs = 0;
    for (const auto& [userId, records] : userHandovers) {
        for (size_t i = 1; i < records.size(); ++i) {
            const auto& prev = *records[i - 1];
            const auto& curr = *records[i];
            double dt = curr.timestamp - prev.timestamp;
            if (dt >= 0.0 && dt <= pingPongThresholdSeconds) {
                // Returned to original satellite
                if (curr.fromSatId == prev.toSatId && curr.toSatId == prev.fromSatId) {
                    pingPongs++;
                }
            }
        }
    }

    kpis.pingPongCount = pingPongs;
    kpis.pingPongRatePercent = (static_cast<double>(pingPongs) / static_cast<double>(kpis.totalAttempts)) * 100.0;

    return kpis;
}

void MetricsEngine::RecordRadioSample(const std::string& userId, const std::string& satId,
                                     double rsrpDbm, double sinrDb, double dopplerShiftHz,
                                     bool inCoverage, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_radioSamples.push_back(RadioSample{
        userId, satId, rsrpDbm, sinrDb, dopplerShiftHz, inCoverage, timestamp
    });

    // Mirror to generic metric samples
    m_records.emplace_back("rsrp", rsrpDbm, timestamp);
    m_records.emplace_back("sinr", sinrDb, timestamp);
    m_records.emplace_back("doppler", dopplerShiftHz, timestamp);
}

RadioKPIs MetricsEngine::CalculateRadioKPIs(double sinrOutageThresholdDb) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    RadioKPIs kpis;
    if (m_radioSamples.empty()) {
        return kpis;
    }

    double sumRsrp = 0.0;
    double sumSinr = 0.0;
    double sumAbsDoppler = 0.0;
    double maxAbsDoppler = 0.0;
    size_t inCoverageCount = 0;
    size_t outageCount = 0;

    std::vector<double> rsrpValues;
    rsrpValues.reserve(m_radioSamples.size());

    for (const auto& s : m_radioSamples) {
        sumRsrp += s.rsrpDbm;
        sumSinr += s.sinrDb;
        rsrpValues.push_back(s.rsrpDbm);

        double absDoppler = std::abs(s.dopplerShiftHz);
        sumAbsDoppler += absDoppler;
        if (absDoppler > maxAbsDoppler) {
            maxAbsDoppler = absDoppler;
        }

        if (s.inCoverage) {
            inCoverageCount++;
        }
        if (!s.inCoverage || s.sinrDb < sinrOutageThresholdDb) {
            outageCount++;
        }
    }

    size_t total = m_radioSamples.size();
    kpis.meanRsrpDbm = sumRsrp / static_cast<double>(total);
    kpis.meanSinrDb = sumSinr / static_cast<double>(total);
    kpis.meanDopplerShiftHz = sumAbsDoppler / static_cast<double>(total);
    kpis.maxDopplerShiftHz = maxAbsDoppler;
    kpis.serviceAvailabilityPercent = (static_cast<double>(inCoverageCount) / static_cast<double>(total)) * 100.0;
    kpis.outageProbabilityPercent = (static_cast<double>(outageCount) / static_cast<double>(total)) * 100.0;

    // Calculate 10th percentile RSRP (cell-edge coverage)
    std::sort(rsrpValues.begin(), rsrpValues.end());
    double k = 0.10 * static_cast<double>(rsrpValues.size() - 1);
    size_t idx = static_cast<size_t>(std::floor(k));
    double f = k - static_cast<double>(idx);
    if (idx + 1 < rsrpValues.size()) {
        kpis.p10RsrpDbm = rsrpValues[idx] + f * (rsrpValues[idx + 1] - rsrpValues[idx]);
    } else {
        kpis.p10RsrpDbm = rsrpValues.back();
    }

    return kpis;
}

void MetricsEngine::RecordPacketSent(uint64_t packetId, size_t sizeBytes, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sentPackets.push_back(PacketSentRecord{packetId, sizeBytes, timestamp});
}

void MetricsEngine::RecordPacketReceived(uint64_t packetId, double latencySeconds, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receivedPackets.push_back(PacketReceivedRecord{packetId, latencySeconds, timestamp});
    m_records.emplace_back("packet_latency", latencySeconds, timestamp);
}

void MetricsEngine::RecordPacketDropped(uint64_t packetId, PacketDropReason reason, double timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_droppedPackets.push_back(PacketDropRecord{packetId, reason, timestamp});
    m_records.emplace_back("packet_drop", 1.0, timestamp);
}

TrafficKPIs MetricsEngine::CalculateTrafficKPIs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    TrafficKPIs kpis;
    kpis.packetsSent = m_sentPackets.size();
    kpis.packetsReceived = m_receivedPackets.size();
    kpis.packetsDropped = m_droppedPackets.size();

    if (kpis.packetsSent > 0) {
        kpis.packetDeliveryRatioPercent = (static_cast<double>(kpis.packetsReceived) / static_cast<double>(kpis.packetsSent)) * 100.0;
    }

    for (const auto& drop : m_droppedPackets) {
        switch (drop.reason) {
            case PacketDropReason::QueueOverflow:
                kpis.dropsQueueOverflow++;
                break;
            case PacketDropReason::TtlExpired:
                kpis.dropsTtlExpired++;
                break;
            case PacketDropReason::RadioChannelLoss:
                kpis.dropsRadioLoss++;
                break;
            case PacketDropReason::NoRoute:
                kpis.dropsNoRoute++;
                break;
        }
    }

    if (m_receivedPackets.empty()) {
        return kpis;
    }

    // Latency statistics
    std::vector<double> latencies;
    latencies.reserve(m_receivedPackets.size());
    double sumLatency = 0.0;

    for (const auto& rx : m_receivedPackets) {
        latencies.push_back(rx.latencySeconds);
        sumLatency += rx.latencySeconds;
    }

    kpis.meanLatencySeconds = sumLatency / static_cast<double>(latencies.size());

    std::sort(latencies.begin(), latencies.end());
    auto getP = [&](double pct) -> double {
        if (latencies.empty()) return 0.0;
        if (latencies.size() == 1) return latencies.front();
        double k = (pct / 100.0) * static_cast<double>(latencies.size() - 1);
        size_t i = static_cast<size_t>(std::floor(k));
        double f = k - static_cast<double>(i);
        if (i + 1 < latencies.size()) {
            return latencies[i] + f * (latencies[i + 1] - latencies[i]);
        }
        return latencies.back();
    };

    kpis.p50LatencySeconds = getP(50.0);
    kpis.p95LatencySeconds = getP(95.0);
    kpis.p99LatencySeconds = getP(99.0);

    // Latency Jitter: mean absolute difference between consecutive received packet delays
    if (m_receivedPackets.size() >= 2) {
        double sumJitter = 0.0;
        for (size_t i = 1; i < m_receivedPackets.size(); ++i) {
            sumJitter += std::abs(m_receivedPackets[i].latencySeconds - m_receivedPackets[i - 1].latencySeconds);
        }
        kpis.jitterSeconds = sumJitter / static_cast<double>(m_receivedPackets.size() - 1);
    }

    // Throughput in Mbps
    // Match received packet IDs with sent packet sizes to compute total bytes received
    std::unordered_map<uint64_t, size_t> packetSizes;
    for (const auto& sent : m_sentPackets) {
        packetSizes[sent.packetId] = sent.sizeBytes;
    }

    size_t totalBytesReceived = 0;
    double minTime = std::numeric_limits<double>::infinity();
    double maxTime = -std::numeric_limits<double>::infinity();

    for (const auto& rx : m_receivedPackets) {
        auto it = packetSizes.find(rx.packetId);
        if (it != packetSizes.end()) {
            totalBytesReceived += it->second;
        } else {
            totalBytesReceived += 1500; // Default Ethernet MTU if untracked
        }
        if (rx.timestamp < minTime) minTime = rx.timestamp;
        if (rx.timestamp > maxTime) maxTime = rx.timestamp;
    }

    double timeSpanSeconds = maxTime - minTime;
    if (timeSpanSeconds > 1e-6) {
        double totalBits = static_cast<double>(totalBytesReceived) * 8.0;
        kpis.throughputMbps = (totalBits / timeSpanSeconds) / 1.0e6;
    }

    return kpis;
}

// ---------------------------------------------------------------------------
// State Management & Export
// ---------------------------------------------------------------------------

std::vector<Metric> MetricsEngine::GetAllRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

bool MetricsEngine::ExportToJson(const std::string& filepath) const {
    (void)filepath;
    // Skeleton implementation (Phase 3)
    return true;
}

void MetricsEngine::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
    m_handoverRecords.clear();
    m_radioSamples.clear();
    m_sentPackets.clear();
    m_receivedPackets.clear();
    m_droppedPackets.clear();
}

} // namespace NtnSim::Metrics
