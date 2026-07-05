# NTN-Sim: Discrete-Event System

This document outlines the event system of the NTN-Sim simulation platform.

## Event Loop Execution

The simulation runs inside a discrete-event execution loop. Instead of advancing time in fixed increments (time-stepped simulation), time advances to the timestamp of the next event in the queue.

```
       +---------------------------------------------+
       |           Simulation Start                  |
       +----------------------+.---------------------+
                              |
                              v
              +---------------+---------------+
              |   Pop Next Event from Queue   | <------+
              +---------------+---------------+        |
                              |                        |
                              | (If Queue Empty or     |
                              |  SimStop reached)      |
                              v                        |
                     /=================\  No           |
                    /   Is Sim Over?    \------------->+
                    \===================/              |
                              | Yes                    |
                              v                        |
                      Execute Callback                 |
                      Update Current Time              |
                      Schedule New Events -------------+
                              |
                              v
       +----------------------+.---------------------+
       |            Simulation Stop                  |
       +---------------------------------------------+
```

## Event Types

NTN-Sim defines a set of critical events modeling different facets of the NTN satellite network:

| Event Type | Priority | Description |
|---|---|---|
| `SimulationStart` | High | Initializes simulation state. |
| `SimulationStop` | High | Triggers cleanup, metrics collection, and exit. |
| `MoveSatellite` | Medium | Periodically updates orbital paths. |
| `MoveUser` | Medium | Periodically updates user mobility positions. |
| `UpdateCoverage` | Medium | Checks elevation angles and recalculates satellite-user visibility. |
| `UpdateSignal` | Medium | Evaluates path loss, Doppler shifts, SINR, and RSSI. |
| `GeneratePacket` | Low | Triggers traffic profile generation at user terminals. |
| `RoutePacket` | Low | Resolves next hop on ISLs or Gateway links. |
| `PacketDelivered` | Low | Records statistics of successful packet transmission. |
| `PacketDropped` | Low | Records packet loss metric and logs drop reasons. |
| `Handover` | High | Evaluates and executes user serving satellite switch. |
| `MetricsUpdate` | Low | Periodically aggregates simulation KPIs. |
| `LoggerFlush` | Low | Commands the async logger to flush its buffers. |

## Event Structure

An `Event` contains:
- `eventId`: Unique incremental identifier.
- `timestamp`: Double-precision simulation time (seconds) at which the event is scheduled to run.
- `priority`: Numeric priority to break ties when timestamps match.
- `type`: The `EventType` enum value.
- `callback`: A `std::function<void()>` executed when the event fires.
- `isCancelled`: Boolean flag allowing scheduled events to be cancelled dynamically (e.g., cancelling a pending handover if the signal stabilizes).
