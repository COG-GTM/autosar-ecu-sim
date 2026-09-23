// Host unit tests for the sensor ramp. Build and run: make test
// The ramp phase must stay bounded so long soak runs neither overflow a
// counter nor produce unbounded temperature/pressure values.
#include "../include/sensor_swc.hpp"
#include <cmath>
#include <cstdio>
#include <limits>

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; }           \
        else { std::printf("ok:   %s\n", msg); }                               \
    } while (0)

static bool nearly(float a, float b) { return std::fabs(a - b) < 1e-4f; }

int main() {
    // Ramp values match the original formula inside one cycle
    {
        SensorData first = sensorSampleAt(0, 20.0f, 1.0f);
        SensorData tenth = sensorSampleAt(10, 20.0f, 1.0f);
        CHECK(nearly(first.temperature, 20.0f) && nearly(first.pressure, 1.0f),
              "phase 0 yields the configured start temperature and 1.0 bar");
        CHECK(nearly(tenth.temperature, 30.0f) && nearly(tenth.pressure, 2.0f),
              "phase 10 ramps temperature and pressure linearly");
    }

    // Phase wraps at the ramp length instead of growing without bound
    {
        CHECK(nextSensorPhase(0) == 1, "phase advances by one");
        CHECK(nextSensorPhase(kSensorRampSteps - 1) == 0,
              "phase wraps back to 0 at the end of a ramp cycle");
    }

    // A long soak keeps the phase and the signal bounded
    {
        unsigned int phase = 0;
        float maxTemp = 0.0f, maxPressure = 0.0f;
        bool bounded = true;
        for (unsigned long n = 0; n < 5'000'000UL; ++n) {
            SensorData d = sensorSampleAt(phase, 20.0f, 1.0f);
            if (phase >= kSensorRampSteps) bounded = false;
            if (d.temperature > maxTemp) maxTemp = d.temperature;
            if (d.pressure > maxPressure) maxPressure = d.pressure;
            phase = nextSensorPhase(phase);
        }
        CHECK(bounded, "phase stays below the ramp length over 5M samples");
        CHECK(nearly(maxTemp, 20.0f + (kSensorRampSteps - 1) * 1.0f),
              "temperature never exceeds one ramp cycle over 5M samples");
        CHECK(nearly(maxPressure, 1.0f + (kSensorRampSteps - 1) * 0.1f),
              "pressure never exceeds one ramp cycle over 5M samples");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
