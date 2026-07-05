# NTN-Sim

**NTN-Sim** is a custom, high-performance, discrete-event simulation platform designed to model 5G Non-Terrestrial Networks (NTN). It simulates LEO satellite constellations, mobile user terminals, signal propagation characteristics (path loss, RSRP, SINR, Doppler), packet queuing, inter-satellite/gateway routing, and satellite handover decisions.

This architecture is designed using modern **C++20** standard library and design principles (SOLID, RAII, Strategy Pattern, Dependency Injection, thread-safe interfaces).

---

## Project Overview

In 5G NTN deployments, satellite nodes move rapidly in Low Earth Orbit (LEO), making cell coverage dynamic and requiring frequent handovers. Simulating these environments at scale requires a dedicated discrete-event scheduler that avoids time-stepped performance bottlenecks and handles millions of events efficiently.

NTN-Sim provides the base skeletal engine layout and interfaces to support high-fidelity satellite, channel, routing, and handover simulation experiments.

---

## Architecture

For a complete architectural overview and flow diagram, please refer to the documentation:
- [System Architecture](file:///c:/TerraLink/ntn-sim/docs/architecture.md)
- [Event-Driven Scheduler System](file:///c:/TerraLink/ntn-sim/docs/event-system.md)
- [Handover Policies and Strategies](file:///c:/TerraLink/ntn-sim/docs/handover.md)
- [Routing Topologies](file:///c:/TerraLink/ntn-sim/docs/routing.md)

---

## Folder Structure

```
ntn-sim/
├── CMakeLists.txt              # Core CMake build instructions
├── README.md                   # Project instructions
├── LICENSE                     # Software license
├── docs/                       # Architectural and design documentation
│   ├── architecture.md
│   ├── event-system.md
│   ├── handover.md
│   └── routing.md
├── config/                     # Configuration parameters
│   └── simulation.json
├── datasets/                   # Satellite TLEs and User coordinate lists
│   ├── satellites/
│   └── users/
├── include/
│   └── ntn-sim/                # Module public headers
│       ├── core/
│       ├── orbit/
│       ├── user/
│       ├── coverage/
│       ├── signal/
│       ├── handover/
│       ├── routing/
│       ├── packet/
│       ├── metrics/
│       ├── logger/
│       ├── api/
│       ├── gateway/
│       └── utils/
├── src/                        # Module implementations (C++ source files)
│       ├── core/
│       ├── orbit/
│       ├── ...
│       └── main.cpp            # Application entry-point
└── tests/                      # GoogleTest integration suite
```

---

## Build Instructions

### Prerequisites
- Operating System: Ubuntu/Linux (or Windows with GCC/MinGW/Clang/MSVC)
- Compiler: `g++` or `clang++` supporting C++20
- Build Tool: CMake (version 3.20 or newer)
- Optional: `ninja-build` for faster compilations

### Compile
Run the following commands in the root of the project (`ntn-sim/`):

```bash
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .
```

### Run
To run the main simulation executable:
```bash
./ntn_sim_bin
```

### Test
To run the unit tests via CTest:
```bash
ctest --output-on-failure
```

---

## Future Roadmap

1. **Orbit calculation integration**: Replace stubs with a SGP4 orbital propagator processing standard TLE datasets.
2. **Channel propagation models**: Introduce rain attenuation, atmospheric absorption, and shadowing models.
3. **Advanced handover algorithms**: Train neural network strategies in PyTorch/TensorRT and bind to the `FutureAIAlgorithm` engine.
4. **Constellation-scale routing**: Implement routing algorithms (e.g. dynamic Dijkstra, load-balanced routing) to route packets across hundreds of satellites.
5. **Real-time REST API**: Enable external monitoring of simulation states using an embedded HTTP server.
