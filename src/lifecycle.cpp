//File: src/lifecycle.cpp
#include "../include/lifecycle.hpp"
#include<csignal>

std::atomic<AppState> sensorState(AppState::INIT);
std::atomic<AppState> controllerState(AppState::INIT);
std::atomic<AppState> diagnosticState(AppState::INIT);

// Async-signal-safe: only lock-free atomic stores, no iostream.
// diagnosticApp is stopped by the ExecutionManager after the controller, not here.
void handleSignal(int){
    sensorState =AppState::SHUTDOWN;
    controllerState =AppState::SHUTDOWN;
}

void setupSignalHandlers(){
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
}
