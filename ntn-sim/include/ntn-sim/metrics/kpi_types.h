#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace NtnSim::Metrics {

/**
 * @brief Categorization of reasons why packets may be dropped in NTN simulation.
 */
enum class PacketDropReason {
    QueueOverflow,    ///< Buffer full at satellite or user queue
    TtlExpired,       ///< Packet time-to-live exceeded
    RadioChannelLoss, ///< Packet corrupted or lost due to poor SINR / outage
    NoRoute           ///< No available inter-satellite link or gateway route
};

/**
 * @brief Telemetry entry for a single satellite handover execution.
 */
struct HandoverRecord {
    std::string userId;
    std::string fromSatId;
    std::string toSatId;
    double timestamp{0.0};
    bool success{true};
    double interruptionTimeSeconds{0.0};
};

/**
 * @brief Aggregated Handover Key Performance Indicators.
 */
struct HandoverKPIs {
    size_t totalAttempts{0};
    size_t totalSuccesses{0};
    size_t totalFailures{0};
    size_t pingPongCount{0};
    double successRatePercent{0.0};
    double pingPongRatePercent{0.0};
    double averageInterruptionTimeSeconds{0.0};
    double maxInterruptionTimeSeconds{0.0};
};

/**
 * @brief Telemetry entry for an RF radio link observation.
 */
struct RadioSample {
    std::string userId;
    std::string satId;
    double rsrpDbm{0.0};
    double sinrDb{0.0};
    double dopplerShiftHz{0.0};
    bool inCoverage{true};
    double timestamp{0.0};
};

/**
 * @brief Aggregated Radio Frequency and Link Quality KPIs.
 */
struct RadioKPIs {
    double meanRsrpDbm{0.0};
    double p10RsrpDbm{0.0};                 ///< 10th percentile RSRP (cell-edge performance)
    double meanSinrDb{0.0};
    double outageProbabilityPercent{0.0};  ///< Percentage of samples with SINR < threshold or out of coverage
    double maxDopplerShiftHz{0.0};
    double meanDopplerShiftHz{0.0};
    double serviceAvailabilityPercent{0.0};///< Percentage of samples where inCoverage == true
};

/**
 * @brief Telemetry entry for a transmitted packet.
 */
struct PacketSentRecord {
    uint64_t packetId{0};
    size_t sizeBytes{0};
    double timestamp{0.0};
};

/**
 * @brief Telemetry entry for a received packet.
 */
struct PacketReceivedRecord {
    uint64_t packetId{0};
    double latencySeconds{0.0};
    double timestamp{0.0};
};

/**
 * @brief Telemetry entry for a dropped packet.
 */
struct PacketDropRecord {
    uint64_t packetId{0};
    PacketDropReason reason{PacketDropReason::QueueOverflow};
    double timestamp{0.0};
};

/**
 * @brief Aggregated Traffic, Latency, and Throughput KPIs.
 */
struct TrafficKPIs {
    size_t packetsSent{0};
    size_t packetsReceived{0};
    size_t packetsDropped{0};
    double packetDeliveryRatioPercent{0.0}; ///< (packetsReceived / packetsSent) * 100
    size_t dropsQueueOverflow{0};
    size_t dropsTtlExpired{0};
    size_t dropsRadioLoss{0};
    size_t dropsNoRoute{0};
    double meanLatencySeconds{0.0};
    double p50LatencySeconds{0.0};          ///< Median latency
    double p95LatencySeconds{0.0};
    double p99LatencySeconds{0.0};
    double jitterSeconds{0.0};              ///< Mean absolute deviation between consecutive packet arrival delays
    double throughputMbps{0.0};
};

} // namespace NtnSim::Metrics
