// File: src/ecu_config.cpp
#include "../include/ecu_config.hpp"
#include <cmath>
#include <limits>

using json = nlohmann::json;

namespace {

const json* child(const json& parent, const char* key) {
    if (!parent.is_object()) {
        return nullptr;
    }
    auto it = parent.find(key);
    return it == parent.end() ? nullptr : &*it;
}

class Reader {
public:
    explicit Reader(ConfigParseResult& result) : result_(result) {}

    // Returns true if `key` was present and assigned to `out`.
    bool number(const json& parent, const char* key, const std::string& path, float& out) {
        const json* value = child(parent, key);
        if (!value) {
            return false;
        }
        if (!value->is_number()) {
            result_.errors.push_back(path + " must be a number");
            return false;
        }
        double d = value->get<double>();
        if (!std::isfinite(d) || std::fabs(d) > std::numeric_limits<float>::max()) {
            result_.errors.push_back(path + " must be a finite number");
            return false;
        }
        out = static_cast<float>(d);
        return true;
    }

    bool integer(const json& parent, const char* key, const std::string& path, int& out) {
        const json* value = child(parent, key);
        if (!value) {
            return false;
        }
        if (!value->is_number_integer() || value->get<long long>() < std::numeric_limits<int>::min() ||
            value->get<long long>() > std::numeric_limits<int>::max()) {
            result_.errors.push_back(path + " must be an integer");
            return false;
        }
        out = value->get<int>();
        return true;
    }

    const json& section(const json& parent, const char* key, const std::string& path) {
        static const json empty = json::object();
        const json* value = child(parent, key);
        if (!value) {
            return empty;
        }
        if (!value->is_object()) {
            result_.errors.push_back(path + " must be an object");
            return empty;
        }
        return *value;
    }

private:
    ConfigParseResult& result_;
};

void readThreshold(Reader& reader, ConfigParseResult& result, const json& controller,
                   const json& thresholds, const char* channel, const char* alias,
                   float& out) {
    std::string path = std::string("controller.thresholds.") + channel;
    if (reader.number(thresholds, channel, path, out)) {
        if (child(controller, alias)) {
            result.warnings.push_back(std::string("controller.") + alias + " ignored; " + path +
                                      " takes precedence");
        }
        return;
    }
    if (reader.number(controller, alias, std::string("controller.") + alias, out)) {
        result.warnings.push_back(std::string("controller.") + alias + " is deprecated; use " +
                                  path);
    }
}

void validateAlarm(ConfigParseResult& result, const char* channel,
                   const HysteresisAlarm::Config& alarm) {
    std::string threshold = std::string("controller.thresholds.") + channel;
    std::string hysteresis = std::string("controller.hysteresis.") + channel;
    if (alarm.threshold < 0.0f) {
        result.errors.push_back(threshold + " must be >= 0");
    }
    if (alarm.hysteresis < 0.0f) {
        result.errors.push_back(hysteresis + " must be >= 0");
    }
    if (alarm.hysteresis >= alarm.threshold) {
        result.errors.push_back(hysteresis + " must be < " + threshold);
    }
}

}  // namespace

ConfigParseResult parseEcuConfig(const json& root) {
    ConfigParseResult result;
    if (!root.is_object()) {
        result.errors.push_back("config root must be an object");
        return result;
    }
    Reader reader(result);
    EcuConfig& config = result.config;

    int schemaVersion = 1;
    reader.integer(root, "schemaVersion", "schemaVersion", schemaVersion);
    if (schemaVersion != 1 && schemaVersion != 2) {
        result.errors.push_back("schemaVersion must be 1 or 2");
    }

    const json& sensor = reader.section(root, "sensor", "sensor");
    reader.number(sensor, "startTemp", "sensor.startTemp", config.sensor.startTemp);
    reader.number(sensor, "tempStep", "sensor.tempStep", config.sensor.tempStep);
    reader.number(sensor, "startPressure", "sensor.startPressure", config.sensor.startPressure);
    reader.number(sensor, "pressureStep", "sensor.pressureStep", config.sensor.pressureStep);
    reader.integer(sensor, "rampSteps", "sensor.rampSteps", config.sensor.rampSteps);
    reader.number(sensor, "tempNoise", "sensor.tempNoise", config.sensor.tempNoise);
    reader.number(sensor, "pressureNoise", "sensor.pressureNoise", config.sensor.pressureNoise);
    reader.integer(sensor, "periodMs", "sensor.periodMs", config.sensor.periodMs);

    const json& controller = reader.section(root, "controller", "controller");
    const json& thresholds = reader.section(controller, "thresholds", "controller.thresholds");
    const json& hysteresis = reader.section(controller, "hysteresis", "controller.hysteresis");
    readThreshold(reader, result, controller, thresholds, "temperature", "warningThreshold",
                  config.controller.temperature.threshold);
    readThreshold(reader, result, controller, thresholds, "pressure", "pressureWarningThreshold",
                  config.controller.pressure.threshold);
    reader.number(hysteresis, "temperature", "controller.hysteresis.temperature",
                  config.controller.temperature.hysteresis);
    reader.number(hysteresis, "pressure", "controller.hysteresis.pressure",
                  config.controller.pressure.hysteresis);
    reader.integer(controller, "periodMs", "controller.periodMs", config.controller.periodMs);

    const json& diagnostics = reader.section(root, "diagnostics", "diagnostics");
    int capacity = static_cast<int>(config.diagnostics.capacity);
    if (reader.integer(diagnostics, "capacity", "diagnostics.capacity", capacity)) {
        if (capacity < 1) {
            result.errors.push_back("diagnostics.capacity must be >= 1");
        } else {
            config.diagnostics.capacity = static_cast<std::size_t>(capacity);
        }
    }
    if (const json* outputFile = child(diagnostics, "outputFile")) {
        if (outputFile->is_string() && !outputFile->get<std::string>().empty()) {
            config.diagnostics.outputFile = outputFile->get<std::string>();
        } else {
            result.errors.push_back("diagnostics.outputFile must be a non-empty string");
        }
    }

    validateAlarm(result, "temperature", config.controller.temperature);
    validateAlarm(result, "pressure", config.controller.pressure);
    if (config.sensor.periodMs <= 0) {
        result.errors.push_back("sensor.periodMs must be > 0");
    }
    if (config.controller.periodMs <= 0) {
        result.errors.push_back("controller.periodMs must be > 0");
    }
    if (config.sensor.rampSteps < 0) {
        result.errors.push_back("sensor.rampSteps must be >= 0");
    }
    return result;
}
