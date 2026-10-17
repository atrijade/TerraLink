#pragma once

#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace NtnSim::Core {

/**
 * @brief Types of events in the discrete-event network simulator.
 */
enum class EventType {
    SimulationStart,
    SimulationStop,
    MoveSatellite,
    MoveUser,
    UpdateCoverage,
    UpdateSignal,
    GeneratePacket,
    RoutePacket,
    PacketDelivered,
    PacketDropped,
    Handover,
    MetricsUpdate,
    LoggerFlush
};

/**
 * @brief Base class for simulator events.
 */
class Event {
public:
    /**
     * @brief Constructs a simulation event.
     * @param id Unique event ID.
     * @param timestamp Time at which the event should run.
     * @param priority Priority to order ties (higher value = higher priority).
     * @param type Event classification.
     * @param callback Code to execute.
     */
    Event(uint64_t id, double timestamp, int priority, EventType type, std::function<void()> callback);
    virtual ~Event();

    uint64_t GetId() const;
    double GetTimestamp() const;
    int GetPriority() const;
    EventType GetType() const;

    /**
     * @brief Fires the event callback.
     */
    void Execute() const;

    /**
     * @brief Cancels the event from execution.
     */
    void Cancel();

    /**
     * @brief Checks if the event has been cancelled.
     */
    bool IsCancelled() const;

private:
    uint64_t m_id;
    double m_timestamp;
    int m_priority;
    EventType m_type;
    std::function<void()> m_callback;
    bool m_isCancelled{false};
};

/**
 * @brief Comparator for ordering events in the scheduler priority queue.
 */
struct EventComparator {
    bool operator()(const std::shared_ptr<Event>& lhs, const std::shared_ptr<Event>& rhs) const {
        if (lhs->GetTimestamp() != rhs->GetTimestamp()) {
            // Lower timestamp fires first (Min-heap behavior)
            return lhs->GetTimestamp() > rhs->GetTimestamp();
        }
        // Higher priority fires first for same timestamp (Max-heap behavior)
        return lhs->GetPriority() < rhs->GetPriority();
    }
};

} // namespace NtnSim::Core
