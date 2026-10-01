// File: include/sensor_swc.hpp
#pragma once
#include "ecu_config.hpp"
#include "message_queue.hpp"
#include "sensor_types.hpp"

SensorData sensorSample(const SensorConfig& config, int index);
void sensorApp(MessageQueue<SensorData>& queue, SensorConfig config);
