// File: src/alarm_state.cpp
#include "../include/alarm_state.hpp"

HysteresisAlarm::HysteresisAlarm(Config config) : config_(config) {}

bool HysteresisAlarm::update(float value) {
    if (state_ == State::Normal && value >= config_.threshold) {
        state_ = State::Alarm;
        return true;
    }
    if (state_ == State::Alarm && value < config_.threshold - config_.hysteresis) {
        state_ = State::Normal;
        return true;
    }
    return false;
}

HysteresisAlarm::State HysteresisAlarm::state() const {
    return state_;
}

const HysteresisAlarm::Config& HysteresisAlarm::config() const {
    return config_;
}
