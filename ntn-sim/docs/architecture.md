# NTN-Sim: System Architecture

This document describes the high-level architecture of NTN-Sim, a custom 5G Non-Terrestrial Network (NTN) discrete-event simulation platform.

## High-Level Architecture

The architecture consists of a central **SimulationController** that coordinates multiple modular engines. Communication between components occurs via interfaces and event dispatching. A central thread-safe **Logger** logs events, while a **MetricsEngine** aggregates Key Performance Indicators (KPIs) like latency, packet drop rate, and handover counts.

```
                     +---------------------------------------+
                     |         SimulationController          |
                     +-------------------+-------------------+
                                         |
                                         v
                     +-------------------+-------------------+
                     |          EventScheduler               |
                     +-------------------+-------------------+
                                         |
         +-------------------------------+-------------------------------+
         |                               |                               |
         v                               v                               v
+--------+--------+             +--------+--------+             +--------+--------+
|   OrbitEngine   |             |   UserEngine    |             | CoverageEngine  |
|                 |             |                 |             |                 |
| (Satellites)    |             | (User Terminals)|             | (Visibility)    |
+--------+--------+             +--------+--------+             +--------+--------+
         |                               |                               |
         +-----------------------+-------+-------------------------------+
                                 |
                                 v
                        +--------+--------+
                        |  SignalEngine   | <------+ (Path loss, SNR, Doppler)
                        +--------+--------+
                                 |
                                 v
                        +--------+--------+             +-----------------+
                        | HandoverEngine  | <---------> | IHandoverAlgo   | (Strategy)
                        +--------+--------+             +-----------------+
                                 |
                                 v
                        +--------+--------+
                        |  PacketEngine   | (Queues, buffering)
                        +--------+--------+
                                 |
                                 v
                        +--------+--------+
                        |  RoutingEngine  | (Constellation routing / ISL)
                        +--------+--------+
                                 |
                                 v
                        +--------+--------+
                        |    Gateway      | (Ground Station link)
                        +--------+--------+
                                 |
                                 +-----------------------+
                                                         |
                                                         v
                                                +--------+--------+
                                                |  MetricsEngine  |
                                                +-----------------+
```

## Architectural Design Principles

1. **SOLID Compliance**:
   - **Single Responsibility**: Each engine manages exactly one domain (e.g., `OrbitEngine` handles satellite movement, `SignalEngine` computes RF characteristics).
   - **Open/Closed**: New handover and routing algorithms can be added by implementing interfaces (`IHandoverAlgorithm` and future routing interfaces) without modifying existing engines.
   - **Liskov Substitution**: Strategy algorithms subclass the algorithm interface cleanly.
   - **Interface Segregation**: Clients of modules depend on narrow, specific interfaces rather than general implementations.
   - **Dependency Inversion**: Higher-level core components depend on abstractions, injected at runtime.
2. **Modern C++20**:
   - Use of `std::unique_ptr` and `std::shared_ptr` to manage lifetimes cleanly (RAII).
   - Concepts and constraints where necessary.
   - Design optimized for fast compilation and type safety.
3. **No Global State**:
   - Singleton pattern is avoided, except for the `Logger` which is thread-safe and controls a dedicated file handle/stdout wrapper.
   - All engines are instantiated in the `SimulationController` and passed as references or shared pointers to other engines using Dependency Injection.
4. **Thread Safety**:
   - Standard mutual exclusion (`std::mutex`) and double-buffered patterns protect shared metrics and logs, preparing the architecture for concurrent event execution in the future.
