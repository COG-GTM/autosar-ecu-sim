// File: include/controller_swc.hpp
#pragma once
#include "alarm_state.hpp"
#include "diagnostic_types.hpp"
#include "ecu_config.hpp"
#include <chrono>
#include <optional>

// Feeds one sample into `alarm`; returns the DiagnosticEvent for a raise/clear transition.
std::optional<DiagnosticEvent> evaluateAlarm(HysteresisAlarm& alarm, Channel channel, float value,
                                             std::chrono::system_clock::time_point ts);

// Design target (B2.0): controllerApp takes the validated config and discovers
// "SensorDataService" / "DiagnosticEventService" queues via ServiceRegistry.
void controllerApp(ControllerConfig config);

// Legacy signature kept until the implementation lands (superseded by the overload above).
void controllerApp(float warningThreshold, int periodMs);
