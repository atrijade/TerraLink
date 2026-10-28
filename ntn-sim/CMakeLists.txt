cmake_minimum_required(VERSION 3.20)
project(ntn-sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile options
if(MSVC)
    add_compile_options(/W4 /WX /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wunused -Woverloaded-virtual)
endif()

# Directories
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${INC_DIR})

# Source files list grouped by module
set(SOURCES
    # Core
    ${SRC_DIR}/core/event.cpp
    ${SRC_DIR}/core/event_scheduler.cpp
    ${SRC_DIR}/core/simulation_controller.cpp
    
    # Orbit
    ${SRC_DIR}/orbit/orbit_state.cpp
    ${SRC_DIR}/orbit/satellite.cpp
    ${SRC_DIR}/orbit/orbit_engine.cpp
    
    # User
    ${SRC_DIR}/user/user.cpp
    ${SRC_DIR}/user/user_engine.cpp
    
    # Coverage
    ${SRC_DIR}/coverage/coverage_info.cpp
    ${SRC_DIR}/coverage/coverage_engine.cpp
    
    # Signal
    ${SRC_DIR}/signal/signal_info.cpp
    ${SRC_DIR}/signal/connection.cpp
    ${SRC_DIR}/signal/signal_engine.cpp
    
    # Handover
    ${SRC_DIR}/handover/best_signal_algorithm.cpp
    ${SRC_DIR}/handover/load_aware_algorithm.cpp
    ${SRC_DIR}/handover/hysteresis_algorithm.cpp
    ${SRC_DIR}/handover/future_ai_algorithm.cpp
    ${SRC_DIR}/handover/handover_engine.cpp
    
    # Packet
    ${SRC_DIR}/packet/packet.cpp
    ${SRC_DIR}/packet/packet_engine.cpp
    
    # Routing
    ${SRC_DIR}/routing/routing_engine.cpp
    
    # Gateway
    ${SRC_DIR}/gateway/gateway.cpp
    ${SRC_DIR}/gateway/gateway_engine.cpp
    
    # Metrics
    ${SRC_DIR}/metrics/metric.cpp
    ${SRC_DIR}/metrics/metrics_engine.cpp
    
    # Logger
    ${SRC_DIR}/logger/logger.cpp
    
    # Api
    ${SRC_DIR}/api/rest_api.cpp
    
    # Utilities
    ${SRC_DIR}/utils/math_utils.cpp
    ${SRC_DIR}/utils/random_generator.cpp
    ${SRC_DIR}/utils/time_utils.cpp
    ${SRC_DIR}/utils/simulation_config.cpp
    ${SRC_DIR}/utils/config_loader.cpp
    ${SRC_DIR}/utils/json_parser.cpp
    ${SRC_DIR}/utils/id_generator.cpp
)

# Header files for IDE grouping
set(HEADERS
    ${INC_DIR}/ntn-sim/core/event.h
    ${INC_DIR}/ntn-sim/core/event_scheduler.h
    ${INC_DIR}/ntn-sim/core/simulation_controller.h
    
    ${INC_DIR}/ntn-sim/orbit/orbit_state.h
    ${INC_DIR}/ntn-sim/orbit/satellite.h
    ${INC_DIR}/ntn-sim/orbit/orbit_engine.h
    
    ${INC_DIR}/ntn-sim/user/user.h
    ${INC_DIR}/ntn-sim/user/user_engine.h
    
    ${INC_DIR}/ntn-sim/coverage/coverage_info.h
    ${INC_DIR}/ntn-sim/coverage/coverage_engine.h
    
    ${INC_DIR}/ntn-sim/signal/signal_info.h
    ${INC_DIR}/ntn-sim/signal/connection.h
    ${INC_DIR}/ntn-sim/signal/signal_engine.h
    
    ${INC_DIR}/ntn-sim/handover/i_handover_algorithm.h
    ${INC_DIR}/ntn-sim/handover/best_signal_algorithm.h
    ${INC_DIR}/ntn-sim/handover/load_aware_algorithm.h
    ${INC_DIR}/ntn-sim/handover/hysteresis_algorithm.h
    ${INC_DIR}/ntn-sim/handover/future_ai_algorithm.h
    ${INC_DIR}/ntn-sim/handover/handover_engine.h
    
    ${INC_DIR}/ntn-sim/packet/packet.h
    ${INC_DIR}/ntn-sim/packet/packet_engine.h
    
    ${INC_DIR}/ntn-sim/routing/routing_engine.h
    
    ${INC_DIR}/ntn-sim/gateway/gateway.h
    ${INC_DIR}/ntn-sim/gateway/gateway_engine.h
    
    ${INC_DIR}/ntn-sim/metrics/metric.h
    ${INC_DIR}/ntn-sim/metrics/metrics_engine.h
    
    ${INC_DIR}/ntn-sim/logger/logger.h
    
    ${INC_DIR}/ntn-sim/api/rest_api.h
    
    ${INC_DIR}/ntn-sim/utils/math_utils.h
    ${INC_DIR}/ntn-sim/utils/random_generator.h
    ${INC_DIR}/ntn-sim/utils/time_utils.h
    ${INC_DIR}/ntn-sim/utils/simulation_config.h
    ${INC_DIR}/ntn-sim/utils/config_loader.h
    ${INC_DIR}/ntn-sim/utils/json_parser.h
    ${INC_DIR}/ntn-sim/utils/id_generator.h
)

# Core Simulator Library
add_library(ntn_sim_lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(ntn_sim_lib PUBLIC ${INC_DIR})

# Main Binary
add_executable(ntn_sim_bin ${SRC_DIR}/main.cpp)
target_link_libraries(ntn_sim_bin PRIVATE ntn_sim_lib)

# Testing Setup
enable_testing()
add_subdirectory(tests)
