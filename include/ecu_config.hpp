//File: include/ecu_config.hpp
#pragma once
#include <string>

// Timing and behavior parameters read from config.json.
struct EcuConfig {
    float startTemp = 0.0f;
    float tempStep = 0.0f;
    float warningThreshold = 0.0f;
    int sensorPeriod = 0;
    int controllerPeriod = 0;
};

// Parses the JSON configuration at `path` into `config`.
// Returns false and fills `error` with a message naming the offending key or
// parse problem instead of throwing.
bool loadEcuConfig(const std::string& path, EcuConfig& config, std::string& error);
