#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/logger/logger.h"
#include <iostream>

int main(int argc, char* argv[]) {
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

    logger.Log(NtnSim::Logger::LogLevel::INFO, "Launching discrete-event simulation run...");
    controller.StartSimulation();

    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "    NTN-Sim Simulation Run Finished     ");
    logger.Log(NtnSim::Logger::LogLevel::INFO, "========================================");

    return 0;
}
