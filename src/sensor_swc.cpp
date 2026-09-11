// File: src/sensor_swc.cpp
#include "../include/sensor_swc.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include<sstream>
#include <thread>
#include <chrono>

extern std::atomic<AppState> sensorState;    

void sensorApp(MessageQueue<SensorData>& queue, float startTemp, float tempStep, int periodMs) {
    sensorState = AppState::RUNNING;
    ServiceRegistry::instance().registerService("SensorDataService", &queue);
    int i = 0;

    while(sensorState != AppState::SHUTDOWN){
        SensorData data;
        data.temperature = startTemp + i * tempStep;
        data.pressure = 1.0f + (i * 0.1f);
        queue.send(data);

        std::ofstream logfile("sensor_log.txt", std::ios::app);
        std::ostringstream line;
        line<<"Temp = "<<data.temperature<<", Pressure = "<<data.pressure;
        std::cout<<("[Sensor] "+line.str()+"\n")<<std::flush;
        if(logfile.is_open()){
            logfile<<line.str()<<std::endl;
        }
        ++i;
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
    std::cout<<"[Sensor] Shutting down. ";
}

