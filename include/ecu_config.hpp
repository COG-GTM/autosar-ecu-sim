// File: include/ecu_config.hpp
#pragma once
#include <istream>
#include <string>

struct EcuConfig {
    float startTemp;
    float tempStep;
    float warningThreshold;
    int sensorPeriod;
    int controllerPeriod;
};

// Parses and validates the ECU configuration. Returns false and fills
// `error` with a message naming the offending key instead of throwing.
bool loadEcuConfig(std::istream& input, EcuConfig& config, std::string& error);
