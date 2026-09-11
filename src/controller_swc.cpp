
// File: src/controller_swc.cpp
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include<sstream>
#include<thread>
#include <chrono>

extern std::atomic<AppState> controllerState;

void controllerApp(float warningThreshold, float pressureThreshold, int periodMs) {
    controllerState = AppState::RUNNING;
    auto queuePtr = static_cast<MessageQueue<SensorData>*>(ServiceRegistry::instance().discoverService("SensorDataService"));
    
    float lastTemp =-1.0f;       
    while(controllerState != AppState::SHUTDOWN){
        SensorData data = queuePtr->receive();
        
        if (data.temperature != lastTemp) {
            std::ofstream logfile("controller_log.txt", std::ios::app);
            std::ostringstream line;
            line << "Temp = " << data.temperature << ", Pressure = " << data.pressure;
            if (data.temperature > warningThreshold) {
                line << " [WARNING: High Temp!]";
            }
            if (data.pressure > pressureThreshold) {
                line << " [WARNING: High Pressure!]";
            }
            std::cout << ("[Controller] " + line.str() + "\n") << std::flush;
            if (logfile.is_open()) {
                logfile << line.str() << std::endl;
            }
        }
        lastTemp = data.temperature;
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Controller] Shutting down."<<std::endl;
}

