//File: src/execution_manager.cpp
#include"../include/execution_manager.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/lifecycle.hpp"
#include "../include/ecu_config.hpp"
#include<string>
#include<thread>
#include <iostream>

bool runExecutionManager(){
    setupSignalHandlers();

    EcuConfig config;
    std::string error;
    if(!loadEcuConfig("config.json", config, error)){
        std::cerr<<"[Execution Manager] Invalid configuration: "<<error<<std::endl;
        return false;
    }

    MessageQueue<SensorData> messageQueue;

    std::thread sensorThread(sensorApp, std::ref(messageQueue), config.startTemp, config.tempStep,
                             config.sensorPeriod);
    std::thread controllerThread(controllerApp, config.warningThreshold, config.controllerPeriod);

    sensorThread.join();
    messageQueue.close();
    controllerThread.join();

    std::cout<<"[Execution Manager] All apps have shut down." <<std::endl;
    return true;
}