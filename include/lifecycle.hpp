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
extern std::atomic<int> shutdownSignal;

extern "C" void handleSignal(int signalNumber);
void setupSignalHandlers();
