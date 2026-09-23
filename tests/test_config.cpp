// Regression test: malformed, partial or wrong-typed config must be reported
// as an error instead of throwing an uncaught exception at ECU startup.
#include "../include/ecu_config.hpp"
#include <cstdio>
#include <sstream>
#include <string>

static int failures = 0;

static void check(bool condition, const std::string& what) {
    std::printf("%s %s\n", condition ? "ok:  " : "FAIL:", what.c_str());
    if (!condition) {
        ++failures;
    }
}

static void expectRejected(const std::string& text, const std::string& what) {
    std::istringstream input(text);
    EcuConfig config;
    std::string error;
    bool ok = loadEcuConfig(input, config, error);
    check(!ok && !error.empty(), what);
}

int main() {
    {
        std::istringstream input(R"({
            "sensor": {"startTemp": 20.0, "tempStep": 1.0, "periodMs": 1000},
            "controller": {"warningThreshold": 25.0, "periodMs": 2000}
        })");
        EcuConfig config;
        std::string error;
        bool ok = loadEcuConfig(input, config, error);
        check(ok && config.startTemp == 20.0f && config.tempStep == 1.0f &&
                  config.warningThreshold == 25.0f && config.sensorPeriod == 1000 &&
                  config.controllerPeriod == 2000,
              "valid config is parsed into every field");
    }

    expectRejected("{ this is not json", "malformed JSON is rejected");
    expectRejected("", "empty file is rejected");
    expectRejected("[1, 2, 3]", "non-object top level is rejected");
    expectRejected(R"({"controller": {"warningThreshold": 25.0, "periodMs": 2000}})",
                   "missing sensor section is rejected");
    expectRejected(R"({
                       "sensor": {"startTemp": 20.0, "periodMs": 1000},
                       "controller": {"warningThreshold": 25.0, "periodMs": 2000}
                   })",
                   "missing sensor.tempStep is rejected");
    expectRejected(R"({
                       "sensor": {"startTemp": "hot", "tempStep": 1.0, "periodMs": 1000},
                       "controller": {"warningThreshold": 25.0, "periodMs": 2000}
                   })",
                   "non-numeric sensor.startTemp is rejected");
    expectRejected(R"({
                       "sensor": {"startTemp": 20.0, "tempStep": 1.0, "periodMs": 1000},
                       "controller": 5
                   })",
                   "non-object controller section is rejected");

    if (failures > 0) {
        std::printf("FAILED (%d failures)\n", failures);
        return 1;
    }
    std::printf("PASSED (0 failures)\n");
    return 0;
}
