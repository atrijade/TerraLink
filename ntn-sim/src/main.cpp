#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/logger/logger.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_keepRunning{true};

static void SignalHandler(int) {
    g_keepRunning.store(false);
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Initialize the logger singleton
    NtnSim::Logger::Logger& logger = NtnSim::Logger::Logger::GetInstance();
    logger.Initialize("ntn-sim.log");
    logger.SetMinLevel(NtnSim::Logger::LogLevel::DEBUG);

    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "          Starting NTN-Sim App          ");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");

    std::string configPath = "config/simulation.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    NtnSim::Core::SimulationController controller;

    logger.Log(NtnSim::Logger::LogLevel::INFO, "Loading configuration from: " + configPath);
    if (!controller.LoadConfiguration(configPath)) {
        logger.Log(NtnSim::Logger::LogLevel::ERROR, "Failed to load configuration file.");
        return 1;
    }

    logger.Log(NtnSim::Logger::LogLevel::INFO, "Initializing simulation engine architecture...");
    controller.InitializeEngines();

    auto* api = controller.GetRestApi();
    if (api) {
        api->StartServer(8080);
        logger.Log(NtnSim::Logger::LogLevel::INFO, "REST API server listening on http://localhost:8080");
        std::cout << "\n=========================================================\n";
        std::cout << "  NTN-Sim REST API listening on http://localhost:8080    \n";
        std::cout << "  Ready for TerraLink Web UI connection!                 \n";
        std::cout << "  (Press Ctrl+C to stop server)                          \n";
        std::cout << "=========================================================\n\n";
    }

    logger.Log(NtnSim::Logger::LogLevel::INFO, "Launching discrete-event simulation run...");
    controller.StartSimulation();

    if (api && api->IsRunning()) {
        std::cout << "Simulation loop ready. REST API server is actively listening on port 8080...\n";
        while (g_keepRunning.load() && api->IsRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        api->StopServer();
    }

    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "    NTN-Sim Simulation Run Finished     ");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");

    return 0;
}
