// File: src/controller_swc.cpp
#include "../include/controller_swc.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <iostream>
#include<fstream>
#include<sstream>
#include <chrono>

extern std::atomic<AppState> controllerState;

std::optional<DiagnosticEvent> evaluateAlarm(HysteresisAlarm& alarm, Channel channel, float value,
                                             std::chrono::system_clock::time_point ts) {
    if (!alarm.update(value)) {
        return std::nullopt;
    }
    EventKind kind = alarm.state() == HysteresisAlarm::State::Alarm ? EventKind::Raised : EventKind::Cleared;
    return DiagnosticEvent{ts, channel, value, alarm.config().threshold, kind};
}

static const char* transitionFlag(const DiagnosticEvent& event) {
    bool raised = event.kind == EventKind::Raised;
    if (event.channel == Channel::Temperature) {
        return raised ? " [WARNING: High Temp!]" : " [CLEARED: Temp]";
    }
    return raised ? " [WARNING: High Pressure!]" : " [CLEARED: Pressure]";
}

void controllerApp(ControllerConfig config) {
    AppState expected = AppState::INIT;
    controllerState.compare_exchange_strong(expected, AppState::RUNNING);
    auto queuePtr = static_cast<MessageQueue<SensorData>*>(ServiceRegistry::instance().discoverService("SensorDataService"));
    auto diagnosticsPtr = static_cast<MessageQueue<DiagnosticEvent>*>(ServiceRegistry::instance().discoverService("DiagnosticEventService"));

    HysteresisAlarm temperatureAlarm(config.temperature);
    HysteresisAlarm pressureAlarm(config.pressure);

    // Every sample is evaluated as soon as it arrives; periodMs only bounds the wait
    // so SHUTDOWN is observed even when the sensor is silent.
    while(controllerState != AppState::SHUTDOWN){
        std::optional<SensorData> received = queuePtr->receiveFor(std::chrono::milliseconds(config.periodMs));
        if (!received) {
            continue;
        }
        SensorData data = *received;
        auto now = std::chrono::system_clock::now();

        std::ostringstream line;
        line << "Temp = " << data.temperature << ", Pressure = " << data.pressure;
        for (auto event : {evaluateAlarm(temperatureAlarm, Channel::Temperature, data.temperature, now),
                           evaluateAlarm(pressureAlarm, Channel::Pressure, data.pressure, now)}) {
            if (event) {
                line << transitionFlag(*event);
                diagnosticsPtr->send(*event);
            }
        }
        std::cout << ("[Controller] " + line.str() + "\n") << std::flush;
        std::ofstream logfile("controller_log.txt", std::ios::app);
        if (logfile.is_open()) {
            logfile << line.str() << std::endl;
        }
    }
    std::cout<<"[Controller] Shutting down."<<std::endl;
}
