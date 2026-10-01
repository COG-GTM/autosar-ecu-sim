// Host unit tests for HysteresisAlarm (SWR-202/203) and controller event generation (SWR-204).
// Build and run: make test
#include "../include/alarm_state.hpp"
#include "../include/controller_swc.hpp"
#include <chrono>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; }           \
        else { std::printf("ok:   %s\n", msg); }                               \
    } while (0)

using State = HysteresisAlarm::State;

int main() {
    // Enter exactly at threshold, not below it
    {
        HysteresisAlarm alarm({3.0f, 0.3f});
        CHECK(!alarm.update(2.99f) && alarm.state() == State::Normal, "below threshold stays Normal");
        CHECK(alarm.update(3.0f) && alarm.state() == State::Alarm, "value == threshold raises");
        CHECK(!alarm.update(3.5f) && alarm.state() == State::Alarm, "further high samples do not re-raise");
    }

    // Clear only strictly below threshold - hysteresis
    {
        HysteresisAlarm alarm({35.0f, 2.0f});
        alarm.update(36.0f);
        CHECK(!alarm.update(34.0f) && alarm.state() == State::Alarm, "inside hysteresis band stays Alarm");
        CHECK(!alarm.update(33.0f) && alarm.state() == State::Alarm, "value == threshold - hysteresis stays Alarm");
        CHECK(alarm.update(32.99f) && alarm.state() == State::Normal, "value < threshold - hysteresis clears");
        CHECK(!alarm.update(34.9f) && alarm.state() == State::Normal, "re-entering band from below stays Normal");
        CHECK(alarm.update(35.0f) && alarm.state() == State::Alarm, "second crossing raises again");
    }

    // Noisy signal around the threshold: no chatter
    {
        HysteresisAlarm alarm({35.0f, 2.0f});
        const std::vector<float> noisy = {34.4f, 35.6f, 34.4f, 35.6f, 34.1f, 35.9f, 33.5f, 34.9f, 33.1f};
        int transitions = 0;
        for (float v : noisy) {
            transitions += alarm.update(v) ? 1 : 0;
        }
        CHECK(transitions == 1 && alarm.state() == State::Alarm, "noise within hysteresis band yields one raise only");
        alarm.update(32.0f);
        CHECK(alarm.state() == State::Normal, "large drop clears after noisy period");
    }

    // evaluateAlarm emits exactly one DiagnosticEvent per transition with the right payload
    {
        HysteresisAlarm alarm({3.0f, 0.3f});
        auto ts = std::chrono::system_clock::now();
        std::vector<DiagnosticEvent> events;
        for (float v : {2.0f, 3.1f, 3.2f, 2.8f, 2.71f, 2.69f, 2.5f, 3.0f}) {
            if (auto event = evaluateAlarm(alarm, Channel::Pressure, v, ts)) {
                events.push_back(*event);
            }
        }
        CHECK(events.size() == 3, "three transitions produce three events");
        CHECK(events.size() == 3 && events[0].kind == EventKind::Raised && events[0].value == 3.1f &&
                  events[1].kind == EventKind::Cleared && events[1].value == 2.69f &&
                  events[2].kind == EventKind::Raised && events[2].value == 3.0f,
              "events carry kind and triggering value in order");
        CHECK(events.size() == 3 && events[0].channel == Channel::Pressure && events[0].threshold == 3.0f &&
                  events[0].ts == ts,
              "events carry channel, configured threshold and timestamp");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
