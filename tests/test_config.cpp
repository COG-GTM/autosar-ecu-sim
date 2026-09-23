// Regression test: malformed, partial or wrong-typed config.json must produce a
// diagnosable error instead of an uncaught nlohmann::json exception at startup.
#include "../include/ecu_config.hpp"
#include <cstdio>
#include <fstream>
#include <string>

static int failures = 0;

static void check(bool condition, const std::string& what) {
    std::printf("%s %s\n", condition ? "ok:  " : "FAIL:", what.c_str());
    if (!condition) {
        ++failures;
    }
}

static std::string writeTempConfig(const std::string& name, const std::string& contents) {
    std::string path = "/tmp/" + name;
    std::ofstream out(path);
    out << contents;
    return path;
}

static void expectRejected(const std::string& name, const std::string& contents,
                           const std::string& expectedSubstring) {
    EcuConfig config;
    std::string error;
    bool ok = loadEcuConfig(writeTempConfig(name, contents), config, error);
    check(!ok && error.find(expectedSubstring) != std::string::npos,
          name + " rejected with error mentioning \"" + expectedSubstring + "\" (got \"" + error +
              "\")");
}

int main() {
    EcuConfig config;
    std::string error;

    bool ok = loadEcuConfig("config.json", config, error);
    check(ok, "repository config.json loads (" + error + ")");
    check(config.sensorPeriod > 0 && config.controllerPeriod > 0,
          "repository config.json yields positive periods");

    check(!loadEcuConfig("/tmp/definitely-missing-config.json", config, error),
          "missing file is reported instead of crashing");

    expectRejected("ecu_malformed.json", "{ \"sensor\": { ", "not valid JSON");
    expectRejected("ecu_partial.json",
                   "{\"sensor\":{\"startTemp\":20.0,\"tempStep\":1.0,\"periodMs\":1000}}",
                   "controller");
    expectRejected("ecu_missing_key.json",
                   "{\"sensor\":{\"startTemp\":20.0,\"periodMs\":1000},"
                   "\"controller\":{\"warningThreshold\":25.0,\"periodMs\":2000}}",
                   "sensor.tempStep");
    expectRejected("ecu_wrong_type.json",
                   "{\"sensor\":{\"startTemp\":\"hot\",\"tempStep\":1.0,\"periodMs\":1000},"
                   "\"controller\":{\"warningThreshold\":25.0,\"periodMs\":2000}}",
                   "sensor.startTemp");
    expectRejected("ecu_bad_period.json",
                   "{\"sensor\":{\"startTemp\":20.0,\"tempStep\":1.0,\"periodMs\":0},"
                   "\"controller\":{\"warningThreshold\":25.0,\"periodMs\":2000}}",
                   "sensor.periodMs");
    expectRejected("ecu_not_object.json", "[1, 2, 3]", "JSON object");

    if (failures > 0) {
        std::printf("FAILED (%d failures)\n", failures);
        return 1;
    }
    std::printf("PASSED (0 failures)\n");
    return 0;
}
