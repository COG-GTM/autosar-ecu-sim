// File: src/sensor_swc.cpp
#include "../include/sensor_swc.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include <thread>
#include <chrono>

extern std::atomic<AppState> sensorState;    

SensorData sensorSampleAt(unsigned int phase, float startTemp, float tempStep) {
    SensorData data;
    data.temperature = startTemp + static_cast<float>(phase) * tempStep;
    data.pressure = 1.0f + (static_cast<float>(phase) * 0.1f);
    return data;
}

unsigned int nextSensorPhase(unsigned int phase) {
    return (phase + 1u) % kSensorRampSteps;
}

void sensorApp(MessageQueue<SensorData>& queue, float startTemp, float tempStep, int periodMs) {
    sensorState = AppState::RUNNING;
    ServiceRegistry::instance().registerService("SensorDataService", &queue);
    unsigned int phase = 0;

    while(sensorState != AppState::SHUTDOWN){
        SensorData data = sensorSampleAt(phase, startTemp, tempStep);
        queue.send(data);

        std::ofstream logfile("sensor_log.txt", std::ios::app);
        std::cout<<"[Sensor] Temp = "<<data.temperature<<", Pressure = "<<data.pressure << std::endl;
        if(logfile.is_open()){
            logfile<<"Temp = "<<data.temperature<<", Pressure = "<<data.pressure<<std::endl;  
        }     
        phase = nextSensorPhase(phase);
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Sensor] Shutting down. ";
}
