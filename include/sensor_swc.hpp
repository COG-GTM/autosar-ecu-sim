// File: include/sensor_swc.hpp
#pragma once
#include "message_queue.hpp"
#include "sensor_types.hpp"

// Number of samples in one ramp cycle; the sensor ramp restarts afterwards so
// values stay bounded and the phase counter never overflows.
constexpr unsigned int kSensorRampSteps = 100;

SensorData sensorSampleAt(unsigned int phase, float startTemp, float tempStep);
unsigned int nextSensorPhase(unsigned int phase);
void sensorApp(MessageQueue<SensorData>& queue,float startTemp, float tempStep, int periodMs);
