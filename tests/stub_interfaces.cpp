// File: tests/stub_interfaces.cpp
// Compile check for the B2.0 interface stubs (AE-9): every design header must be
// self-contained and compile under -std=c++17 without needing its implementation.
#include "../include/alarm_state.hpp"
#include "../include/diagnostic_types.hpp"
#include "../include/diagnostic_swc.hpp"
#include "../include/ecu_config.hpp"
#include "../include/controller_swc.hpp"

int main() {
    // Types instantiate; declared-but-unimplemented functions are not called.
    HysteresisAlarm::Config alarmConfig{35.0f, 2.0f};
    DiagnosticEvent event{std::chrono::system_clock::now(), Channel::Temperature, 36.0f,
                          alarmConfig.threshold, EventKind::Raised};
    DiagnosticConfig diagConfig;
    EcuConfig ecuConfig;
    return (event.kind == EventKind::Raised && diagConfig.capacity == ecuConfig.diagnostics.capacity)
               ? 0
               : 1;
}
