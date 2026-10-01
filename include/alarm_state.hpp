// File: include/alarm_state.hpp
#pragma once

// Two-state alarm with hysteresis: Normal -> Alarm when value >= threshold,
// Alarm -> Normal only when value < threshold - hysteresis.
class HysteresisAlarm {
public:
    struct Config {
        float threshold;
        float hysteresis;
    };

    enum class State {
        Normal,
        Alarm
    };

    explicit HysteresisAlarm(Config config);

    // Returns true when this sample caused a state transition.
    bool update(float value);
    State state() const;
    const Config& config() const;

private:
    Config config_;
    State state_ = State::Normal;
};
