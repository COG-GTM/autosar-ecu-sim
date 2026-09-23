#include "../include/rte.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include "../include/config_loader.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include<unordered_map>

using json=nlohmann::json;
using namespace std::chrono;

void runECU(){
    float startTemp = 0.0f;
    float tempStep = 0.0f;
    float warningThreshold = 0.0f;
    int durationMs = 0;
    json schedule = json::array();
    std::unordered_map<std::string, int> periodMap;
    std::unordered_map<std::string, int> timeElapsed;

    try{
        const json config = config::load("config.json");
        startTemp = config::requireFloat(config, "sensor.startTemp");
        tempStep = config::requireFloat(config, "sensor.tempStep");
        warningThreshold = config::requireFloat(config, "controller.warningThreshold");
        durationMs = config::requireInt(config, "rte.durationMs");

        schedule = config::requireArray(config, "schedule");
        for (const auto& task: schedule){
            const std::string name = config::requireString(task, "task");
            periodMap[name] = config::requireInt(task, "periodMs");
            timeElapsed[name] = 0;
        }
    }
    catch(const config::ConfigError& e){
        std::cerr << "[RTE] Configuration error: " << e.what() << std::endl;
        return;
    }

    SharedMemory shm;
    float lastTemp = -1.0f;
    auto start=steady_clock::now();

    while(duration_cast<milliseconds>(steady_clock::now()-start).count()<durationMs){
        for(auto& task:schedule){
            const std::string name = task["task"].get<std::string>();
            int& elapsed =timeElapsed[name];
            int period =periodMap[name];

            if(elapsed >= period){
                if(name == "sensor"){
                    runSensorSWC(shm, startTemp, tempStep);
                }
                else if(name == "controller"){
                    SensorData current = shm.readData();
                    runControllerSWC(shm, lastTemp, warningThreshold);
                    lastTemp = current.temperature;
                }
                elapsed = 0;
            }
            elapsed += 10; // Assuming each loop iteration takes 10ms
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep for 10ms to simulate time passing
    }
}