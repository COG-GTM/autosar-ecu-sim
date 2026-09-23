// File: src/controller_swc.cpp
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <algorithm>
#include <iostream>
#include<fstream>
#include<thread>
#include <chrono>

extern std::atomic<AppState> controllerState;

void controllerApp(float warningThreshold, int periodMs) {
    // Only INIT -> RUNNING, so a shutdown requested before this thread starts is not erased.
    AppState expected = AppState::INIT;
    if (!controllerState.compare_exchange_strong(expected, AppState::RUNNING)
        && expected == AppState::SHUTDOWN) {
        std::cout << "[Controller] Shutting down." << std::endl;
        return;
    }

    // Bounded discovery: retry until the service appears or shutdown is requested,
    // so a missing or late producer can never park this thread forever.
    const auto discoveryPoll = std::chrono::milliseconds(std::max(1, std::min(periodMs, 100)));
    MessageQueue<SensorData>* queuePtr = nullptr;
    while (controllerState != AppState::SHUTDOWN) {
        queuePtr = ServiceRegistry::instance().discoverServiceAs<MessageQueue<SensorData>>(
            "SensorDataService", discoveryPoll);
        if (queuePtr) {
            break;
        }
    }
    if (!queuePtr) {
        std::cout << "[Controller] Shutting down." << std::endl;
        return;
    }

    float lastTemp =-1.0f;       
    while(controllerState != AppState::SHUTDOWN){
        std::optional<SensorData> received = queuePtr->receiveFor(std::chrono::milliseconds(periodMs));
        if (!received) {
            continue;
        }
        SensorData data = *received;
        
        if (data.temperature != lastTemp) {
            std::ofstream logfile("controller_log.txt", std::ios::app);
            std::cout << "[Controller] Temp = " << data.temperature << ", Pressure = " << data.pressure;
            if (logfile.is_open()) {
                logfile << "Temp = " << data.temperature << ", Pressure = " << data.pressure;
            }

            if (data.temperature > warningThreshold) {
                std::cout << " [WARNING: High Temp!]";
                if (logfile.is_open()) {
                    logfile << " [WARNING: High Temp!]";
                }
            }
            std::cout << std::endl;
            if (logfile.is_open()) {
                logfile << std::endl;
            }
        }
        lastTemp = data.temperature;
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Controller] Shutting down."<<std::endl;
}
