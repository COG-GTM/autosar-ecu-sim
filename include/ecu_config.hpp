// File: include/ecu_config.hpp
#pragma once
#include "alarm_state.hpp"
#include "diagnostic_swc.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct SensorConfig {
    float startTemp = 20.0f;
    float tempStep = 1.0f;
    float startPressure = 1.0f;
    float pressureStep = 0.1f;
    // 0 = monotonic ramp; N > 0 = triangle wave rising for N samples, then falling for N.
    int rampSteps = 0;
    // Alternating +/- offset added to every sample to emulate measurement noise.
    float tempNoise = 0.0f;
    float pressureNoise = 0.0f;
    int periodMs = 1000;
};

struct ControllerConfig {
    HysteresisAlarm::Config temperature{35.0f, 2.0f};
    HysteresisAlarm::Config pressure{3.0f, 0.3f};
    int periodMs = 1000;
};

struct EcuConfig {
    SensorConfig sensor;
    ControllerConfig controller;
    DiagnosticConfig diagnostics;
};

struct ConfigParseResult {
    EcuConfig config;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    bool ok() const { return errors.empty(); }
};

// Parses config schema v2 (with v1 flat aliases) and validates it.
ConfigParseResult parseEcuConfig(const nlohmann::json& root);
