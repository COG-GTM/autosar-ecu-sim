//File: src/lifecycle.cpp
#include "../include/lifecycle.hpp"
#include<csignal>

std::atomic<AppState> sensorState(AppState::INIT);
std::atomic<AppState> controllerState(AppState::INIT);
std::atomic<int> shutdownSignal(0);

// Async-signal-safe: only lock-free atomic stores, no iostream.
extern "C" void handleSignal(int signalNumber){
    shutdownSignal.store(signalNumber, std::memory_order_relaxed);
    sensorState.store(AppState::SHUTDOWN, std::memory_order_relaxed);
    controllerState.store(AppState::SHUTDOWN, std::memory_order_relaxed);
}

void setupSignalHandlers(){
    struct sigaction action{};
    action.sa_handler = handleSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}
