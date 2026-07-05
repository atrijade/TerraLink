#include <gtest/gtest.h>
#include "ntn-sim/logger/logger.h"
#include <thread>
#include <vector>

TEST(LoggerTest, ThreadSafeSingletonLogCalls) {
    NtnSim::Logger::Logger& logger = NtnSim::Logger::Logger::GetInstance();
    logger.Initialize("test-ntn-sim.log");
    logger.SetMinLevel(NtnSim::Logger::LogLevel::DEBUG);
    
    // Test logging in a multithreaded environment
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&logger, i]() {
            logger.Log(NtnSim::Logger::LogLevel::DEBUG, "Message from thread " + std::to_string(i));
            logger.Log(NtnSim::Logger::LogLevel::INFO, "Info message from thread " + std::to_string(i));
        });
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    SUCCEED();
}
