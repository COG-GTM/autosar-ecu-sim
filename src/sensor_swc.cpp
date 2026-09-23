// File: src/sensor_swc.cpp
#include "../include/sensor_swc.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include <thread>
#include <chrono>

extern std::atomic<AppState> sensorState;    

void sensorApp(MessageQueue<SensorData>& queue, float startTemp, float tempStep, int periodMs) {
    sensorState = AppState::RUNNING;
    ServiceRegistry::instance().registerService("SensorDataService", &queue);
    int i = 0;

    std::ofstream logfile("sensor_log.txt", std::ios::app);
    if(!logfile.is_open()){
        std::cerr<<"[Sensor] Unable to open sensor_log.txt; continuing without file logging."<<std::endl;
    }

    while(sensorState != AppState::SHUTDOWN){
        SensorData data;
        data.temperature = startTemp + i * tempStep;
        data.pressure = 1.0f + (i * 0.1f);
        queue.send(data);

        std::cout<<"[Sensor] Temp = "<<data.temperature<<", Pressure = "<<data.pressure << std::endl;
        if(logfile.is_open()){
            logfile<<"Temp = "<<data.temperature<<", Pressure = "<<data.pressure<<std::endl;  
        }     
        ++i;
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Sensor] Shutting down. ";
}

