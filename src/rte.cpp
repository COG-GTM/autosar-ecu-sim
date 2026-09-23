#include "../include/rte.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/controller_swc.hpp"
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include<fstream>
#include<unordered_map>

using json=nlohmann::json;
using namespace std::chrono;

void runECU(){
    std::ifstream configFile("config.json");
    if(!configFile.is_open()){
        std::cerr << "Failed to open config.json" << std::endl;
        return;
    }
    json config;
    configFile >> config;

    float startTemp = config["sensor"]["startTemp"];
    float tempStep = config["sensor"]["tempStep"];
    float warningThreshold = config["controller"]["warningThreshold"];
    int durationMs= config["rte"]["durationMs"];

    auto schedule =config["schedule"];
    std::unordered_map<std::string, int> periodMap;
    std::unordered_map<std::string, int> timeElapsed;
    for (const auto& task: schedule){
        periodMap[task["task"]] =task["periodMs"];
        timeElapsed[task["task"]]=0;
    }

    SharedMemory shm;
    float lastTemp = -1.0f;
    auto start=steady_clock::now();

    while(duration_cast<milliseconds>(steady_clock::now()-start).count()<durationMs){
        for(auto& task:schedule){
            std::string name =task["task"];
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