//File: include/lifecycle.hpp
#pragma once
#include <atomic>

enum class AppState{
    INIT,
    RUNNING,
    SHUTDOWN
};

extern std::atomic<AppState> sensorState;
extern std::atomic<AppState> controllerState;
extern std::atomic<AppState> diagnosticState;

void handleSignal(int signal);
void setupSignalHandlers();
