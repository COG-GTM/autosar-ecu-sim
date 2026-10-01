//File: include/lifecycle.hpp
#pragma once
#include <atomic>

enum class AppState{
    INIT,
    RUNNING,
    SHUTDOWN
};

// SWCs only move INIT -> RUNNING, so a SHUTDOWN requested during start-up is never overwritten.
extern std::atomic<AppState> sensorState;
extern std::atomic<AppState> controllerState;
extern std::atomic<AppState> diagnosticState;

void handleSignal(int signal);
void setupSignalHandlers();
