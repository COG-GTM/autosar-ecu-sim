// Host unit tests for config schema v2 parsing and validation (SWR-200/201).
// Build and run: make test
#include "../include/ecu_config.hpp"
#include <cmath>
#include <cstdio>
#include <string>

using json = nlohmann::json;

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; }           \
        else { std::printf("ok:   %s\n", msg); }                               \
    } while (0)

static bool hasError(const ConfigParseResult& r, const std::string& needle) {
    for (const std::string& e : r.errors) {
        if (e.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int main() {
    // Defaults when sections are missing
    {
        ConfigParseResult r = parseEcuConfig(json::object());
        const ControllerConfig& c = r.config.controller;
        CHECK(r.ok(), "empty config is valid");
        CHECK(c.temperature.threshold == 35.0f && c.temperature.hysteresis == 2.0f,
              "temperature defaults 35 / 2.0");
        CHECK(c.pressure.threshold == 3.0f && c.pressure.hysteresis == 0.3f, "pressure defaults 3.0 / 0.3");
        CHECK(r.config.diagnostics.capacity == 256 &&
                  r.config.diagnostics.outputFile == "diagnostics/events.json",
              "diagnostics defaults 256 / diagnostics/events.json");
    }

    // Full v2 document
    {
        json cfg = json::parse(R"({
            "schemaVersion": 2,
            "sensor": {"startTemp": 30, "tempStep": 0.5, "rampSteps": 24, "periodMs": 500},
            "controller": {"thresholds": {"temperature": 40, "pressure": 2.5},
                           "hysteresis": {"temperature": 1.5, "pressure": 0.2}, "periodMs": 250},
            "diagnostics": {"capacity": 8, "outputFile": "out/ev.json"}})");
        ConfigParseResult r = parseEcuConfig(cfg);
        CHECK(r.ok() && r.warnings.empty(), "v2 config parses without warnings");
        CHECK(r.config.controller.temperature.threshold == 40.0f &&
                  r.config.controller.pressure.hysteresis == 0.2f && r.config.controller.periodMs == 250,
              "v2 controller values applied");
        CHECK(r.config.sensor.rampSteps == 24 && r.config.sensor.periodMs == 500, "v2 sensor values applied");
        CHECK(r.config.diagnostics.capacity == 8 && r.config.diagnostics.outputFile == "out/ev.json",
              "v2 diagnostics values applied");
    }

    // Deprecated v1 aliases
    {
        json cfg = json::parse(R"({"controller": {"warningThreshold": 25, "pressureWarningThreshold": 4}})");
        ConfigParseResult r = parseEcuConfig(cfg);
        CHECK(r.ok(), "v1 aliases accepted");
        CHECK(r.config.controller.temperature.threshold == 25.0f &&
                  r.config.controller.pressure.threshold == 4.0f,
              "aliases map to thresholds.temperature / thresholds.pressure");
        CHECK(r.warnings.size() == 2, "each alias produces a deprecation warning");
    }
    {
        json cfg = json::parse(R"({"controller": {"warningThreshold": 25, "thresholds": {"temperature": 40}}})");
        ConfigParseResult r = parseEcuConfig(cfg);
        CHECK(r.ok() && r.config.controller.temperature.threshold == 40.0f && r.warnings.size() == 1,
              "nested v2 key wins over alias");
    }

    // Validation failures
    {
        json cfg = json::parse(R"({"controller": {"thresholds": {"pressure": 3.0}, "hysteresis": {"pressure": 3.0}}})");
        CHECK(hasError(parseEcuConfig(cfg), "controller.hysteresis.pressure must be <"),
              "hysteresis >= threshold rejected");
    }
    {
        json cfg = json::parse(R"({"controller": {"thresholds": {"temperature": -1}}})");
        CHECK(hasError(parseEcuConfig(cfg), "controller.thresholds.temperature must be >= 0"),
              "negative threshold rejected");
    }
    {
        json cfg = json::parse(R"({"controller": {"hysteresis": {"temperature": -0.5}}})");
        CHECK(hasError(parseEcuConfig(cfg), "controller.hysteresis.temperature must be >= 0"),
              "negative hysteresis rejected");
    }
    {
        json cfg = json::object();
        cfg["controller"]["thresholds"]["pressure"] = std::nan("");
        CHECK(hasError(parseEcuConfig(cfg), "controller.thresholds.pressure must be a finite number"),
              "NaN threshold rejected");
    }
    {
        json cfg = json::parse(R"({"controller": {"pressureWarningThreshold": "high"}})");
        CHECK(hasError(parseEcuConfig(cfg), "controller.pressureWarningThreshold must be a number"),
              "non-numeric alias rejected");
    }
    {
        json cfg = json::parse(R"({"diagnostics": {"capacity": 0}, "controller": {"periodMs": 0}})");
        ConfigParseResult r = parseEcuConfig(cfg);
        CHECK(hasError(r, "diagnostics.capacity must be >= 1") && hasError(r, "controller.periodMs must be > 0"),
              "zero capacity and period rejected");
    }
    {
        json cfg = json::parse(R"({"schemaVersion": 3})");
        CHECK(hasError(parseEcuConfig(cfg), "schemaVersion"), "unknown schemaVersion rejected");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
