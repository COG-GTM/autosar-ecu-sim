
// File: src/controller_swc.cpp
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include<thread>
#include <chrono>

extern std::atomic<AppState> controllerState;

void controllerApp(float warningThreshold, int periodMs) {
    controllerState = AppState::RUNNING;
    auto queuePtr = static_cast<MessageQueue<SensorData>*>(ServiceRegistry::instance().discoverService("SensorDataService"));
    
    float lastTemp =-1.0f;       
    while(controllerState != AppState::SHUTDOWN){
        SensorData data = queuePtr->receive();
        
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

