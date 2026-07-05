include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ntn_sim_tests
    test_simulation_controller.cpp
    test_event_scheduler.cpp
    test_orbit_engine.cpp
    test_user_engine.cpp
    test_coverage_engine.cpp
    test_signal_engine.cpp
    test_handover_engine.cpp
    test_packet_engine.cpp
    test_routing_engine.cpp
    test_metrics_engine.cpp
    test_logger.cpp
    test_utils.cpp
)

target_link_libraries(ntn_sim_tests PRIVATE ntn_sim_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(ntn_sim_tests)
