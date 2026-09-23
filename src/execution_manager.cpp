//File: src/execution_manager.cpp
#include"../include/execution_manager.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/lifecycle.hpp"
#include "../include/config_loader.hpp"
#include<thread>
#include <iostream>

void runExecutionManager(){
    setupSignalHandlers();

    float startTemp=0.0f;
    float tempStep=0.0f;
    float warningThreshold=0.0f;
    int sensorPeriod=0;
    int controllerPeriod=0;

    try{
        const nlohmann::json config= config::load("config.json");
        startTemp= config::requireFloat(config, "sensor.startTemp");
        tempStep= config::requireFloat(config, "sensor.tempStep");
        warningThreshold= config::requireFloat(config, "controller.warningThreshold");
        sensorPeriod= config::requireInt(config, "sensor.periodMs");
        controllerPeriod= config::requireInt(config, "controller.periodMs");
    }
    catch(const config::ConfigError& e){
        std::cerr<<"[Execution Manager] Configuration error: "<<e.what()<<std::endl;
        return;
    }


    MessageQueue<SensorData> messageQueue;

    std::thread sensorThread(sensorApp, std::ref(messageQueue), startTemp, tempStep, sensorPeriod);
    std::thread controllerThread(controllerApp, warningThreshold, controllerPeriod);

    sensorThread.join();
    controllerThread.join();

    std::cout<<"[Execution Manager] All apps have shut down." <<std::endl;   
}