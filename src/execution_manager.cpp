//File: src/execution_manager.cpp
#include"../include/execution_manager.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/lifecycle.hpp"
#include <nlohmann/json.hpp>
#include<fstream>
#include<thread>
#include <iostream>

using json=nlohmann::json;

void runExecutionManager(){
    setupSignalHandlers();

    std::ifstream configFile("config.json");
    if(!configFile.is_open()){
        std::cerr<<"Failed to open config.json"<<std::endl;
        return;
    }
    json config;
    configFile>>config;

    float startTemp= config["sensor"]["startTemp"];
    float tempStep= config["sensor"]["tempStep"];
    float warningThreshold= config["controller"]["warningThreshold"];
    int sensorPeriod= config["sensor"]["periodMs"];
    int controllerPeriod= config["controller"]["periodMs"];
    
    MessageQueue<SensorData> messageQueue;

    std::thread sensorThread(sensorApp, std::ref(messageQueue), startTemp, tempStep, sensorPeriod);
    std::thread controllerThread(controllerApp, warningThreshold, controllerPeriod);

    sensorThread.join();
    controllerThread.join();

    std::cout<<"[Execution Manager] All apps have shut down." <<std::endl;   
}