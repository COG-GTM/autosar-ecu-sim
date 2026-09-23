// File: include/controller_swc.hpp
#pragma once

// Smallest temperature delta the controller treats as a real change.
constexpr float kTemperatureChangeEpsilon = 0.01f;

bool temperatureChanged(float current, float previous);

void controllerApp(float warningThreshold, int periodMs);
