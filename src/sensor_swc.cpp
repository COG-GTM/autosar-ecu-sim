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

SensorData sensorSample(const SensorConfig& config, long long index) {
    long long phase = index;
    if (config.rampSteps > 0) {
        long long period = 2LL * config.rampSteps;
        phase = index % period;
        if (phase > config.rampSteps) {
            phase = period - phase;
        }
    }
    float noiseSign = (index % 2 == 0) ? 1.0f : -1.0f;
    SensorData data;
    data.temperature = config.startTemp + static_cast<float>(phase) * config.tempStep + noiseSign * config.tempNoise;
    data.pressure = config.startPressure + static_cast<float>(phase) * config.pressureStep + noiseSign * config.pressureNoise;
    return data;
}

void sensorApp(MessageQueue<SensorData>& queue, SensorConfig config) {
    AppState expected = AppState::INIT;
    sensorState.compare_exchange_strong(expected, AppState::RUNNING);
    ServiceRegistry::instance().registerService("SensorDataService", &queue);
    long long i = 0;

    while(sensorState != AppState::SHUTDOWN){
        SensorData data = sensorSample(config, i);
        queue.send(data);

        std::ofstream logfile("sensor_log.txt", std::ios::app);
        std::ostringstream line;
        line<<"Temp = "<<data.temperature<<", Pressure = "<<data.pressure;
        std::cout<<("[Sensor] "+line.str()+"\n")<<std::flush;
        if(logfile.is_open()){
            logfile<<line.str()<<std::endl;
        }
        ++i;
        std::this_thread::sleep_for(std::chrono::milliseconds(config.periodMs));
    }
    std::cout<<"[Sensor] Shutting down. ";
}

