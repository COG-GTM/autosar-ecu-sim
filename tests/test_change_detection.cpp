// File: tests/test_change_detection.cpp
#include "../include/controller_swc.hpp"
#include <cassert>
#include <iostream>

int main() {
    // Identical samples are not a change.
    assert(!temperatureChanged(20.0f, 20.0f));

    // Floating-point noise below the sensor resolution is not a change.
    assert(!temperatureChanged(20.0f + 1e-6f, 20.0f));
    assert(!temperatureChanged(0.1f + 0.2f, 0.3f));

    // A step at or above the sensor resolution is a change, in both directions.
    assert(temperatureChanged(20.5f, 20.0f));
    assert(temperatureChanged(20.0f, 20.5f));

    // Deltas straddling the epsilon boundary.
    assert(!temperatureChanged(20.0f + kTemperatureChangeEpsilon / 2.0f, 20.0f));
    assert(temperatureChanged(20.0f + kTemperatureChangeEpsilon * 2.0f, 20.0f));

    std::cout << "test_change_detection passed" << std::endl;
    return 0;
}
