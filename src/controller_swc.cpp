// File: src/controller_swc.cpp
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <cmath>
#include <iostream>
#include<fstream>
#include<thread>
#include <chrono>

extern std::atomic<AppState> controllerState;

bool temperatureChanged(float current, float previous) {
    return std::fabs(current - previous) > kTemperatureChangeEpsilon;
}

void controllerApp(float warningThreshold, int periodMs) {
    controllerState = AppState::RUNNING;
    auto queuePtr = static_cast<MessageQueue<SensorData>*>(ServiceRegistry::instance().discoverService("SensorDataService"));
    
    float lastTemp = 0.0f;
    bool hasLastTemp = false;
    while(controllerState != AppState::SHUTDOWN){
        std::optional<SensorData> received = queuePtr->receiveFor(std::chrono::milliseconds(periodMs));
        if (!received) {
            continue;
        }
        SensorData data = *received;
        
        if (!hasLastTemp || temperatureChanged(data.temperature, lastTemp)) {
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
            lastTemp = data.temperature;
            hasLastTemp = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Controller] Shutting down."<<std::endl;
}
