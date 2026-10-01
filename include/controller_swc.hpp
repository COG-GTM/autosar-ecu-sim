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

void controllerApp(ControllerConfig config);
