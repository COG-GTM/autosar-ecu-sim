//File: src/execution_manager.cpp
#include"../include/execution_manager.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include "../include/diagnostic_swc.hpp"
#include "../include/ecu_config.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <nlohmann/json.hpp>
#include<fstream>
#include<thread>
#include <iostream>

using json=nlohmann::json;

int runExecutionManager(const std::string& configPath){
    setupSignalHandlers();

    std::ifstream configFile(configPath);
    if(!configFile.is_open()){
        std::cerr<<"Failed to open "<<configPath<<std::endl;
        return 1;
    }
    json root = json::parse(configFile, nullptr, false);
    if (root.is_discarded()) {
        std::cerr << "[Execution Manager] Config error: " << configPath << " is not valid JSON" << std::endl;
        return 1;
    }
    ConfigParseResult parsed = parseEcuConfig(root);
    for (const std::string& warning : parsed.warnings) {
        std::cerr << "[Execution Manager] Config warning: " << warning << std::endl;
    }
    if (!parsed.ok()) {
        for (const std::string& error : parsed.errors) {
            std::cerr << "[Execution Manager] Config error: " << error << std::endl;
        }
        std::cerr << "[Execution Manager] Refusing to start with invalid " << configPath << std::endl;
        return 1;
    }
    const EcuConfig& config = parsed.config;

    MessageQueue<SensorData> messageQueue;
    MessageQueue<DiagnosticEvent> diagnosticQueue;

    // Start-up: diagnostics must be registered before the controller can raise events.
    std::thread diagnosticThread(diagnosticApp, std::ref(diagnosticQueue), config.diagnostics);
    ServiceRegistry::instance().discoverService("DiagnosticEventService");
    std::thread sensorThread(sensorApp, std::ref(messageQueue), config.sensor);
    std::thread controllerThread(controllerApp, config.controller);

    // Shutdown: sensor -> controller -> diagnostics (dumps events last).
    sensorThread.join();
    messageQueue.close();
    controllerThread.join();
    diagnosticQueue.close();
    diagnosticThread.join();

    std::cout<<"[Execution Manager] All apps have shut down." <<std::endl;
    return 0;
}
