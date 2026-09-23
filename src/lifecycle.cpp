//File: src/lifecycle.cpp
#include "../include/lifecycle.hpp"
#include<csignal>
#include<iostream>

std::atomic<AppState> sensorState(AppState::INIT);
std::atomic<AppState> controllerState(AppState::INIT);

void handleSignal(int signal){
    std::cout<<"\n [Execution Manager] Shutdown signal received."<< std::endl;
    sensorState =AppState::SHUTDOWN;
    controllerState =AppState::SHUTDOWN;
}

void setupSignalHandlers(){
    std::signal(SIGINT, handleSignal);
}