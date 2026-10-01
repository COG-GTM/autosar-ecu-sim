// File: include/diagnostic_types.hpp
#pragma once
#include <chrono>

enum class Channel {
    Temperature,
    Pressure
};

enum class EventKind {
    Raised,
    Cleared
};

struct DiagnosticEvent {
    std::chrono::system_clock::time_point ts;
    Channel channel;
    float value;
    float threshold;
    EventKind kind;
};
