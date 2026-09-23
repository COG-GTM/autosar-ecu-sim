//File: src/ecu_config.cpp
#include "../include/ecu_config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace {

const json* findObject(const json& parent, const std::string& key, std::string& error) {
    auto it = parent.find(key);
    if (it == parent.end()) {
        error = "missing section \"" + key + "\"";
        return nullptr;
    }
    if (!it->is_object()) {
        error = "section \"" + key + "\" must be an object";
        return nullptr;
    }
    return &(*it);
}

bool readNumber(const json& section, const std::string& sectionName, const std::string& key,
                double& value, std::string& error) {
    auto it = section.find(key);
    if (it == section.end()) {
        error = "missing key \"" + sectionName + "." + key + "\"";
        return false;
    }
    if (!it->is_number()) {
        error = "key \"" + sectionName + "." + key + "\" must be a number";
        return false;
    }
    value = it->get<double>();
    return true;
}

bool readPeriod(const json& section, const std::string& sectionName, int& value, std::string& error) {
    double raw = 0.0;
    if (!readNumber(section, sectionName, "periodMs", raw, error)) {
        return false;
    }
    if (raw <= 0.0) {
        error = "key \"" + sectionName + ".periodMs\" must be greater than 0";
        return false;
    }
    value = static_cast<int>(raw);
    return true;
}

}  // namespace

bool loadEcuConfig(const std::string& path, EcuConfig& config, std::string& error) {
    std::ifstream configFile(path);
    if (!configFile.is_open()) {
        error = "failed to open " + path;
        return false;
    }

    json root = json::parse(configFile, nullptr, false);
    if (root.is_discarded()) {
        error = path + " is not valid JSON";
        return false;
    }
    if (!root.is_object()) {
        error = path + " must contain a JSON object";
        return false;
    }

    const json* sensor = findObject(root, "sensor", error);
    if (sensor == nullptr) {
        return false;
    }
    const json* controller = findObject(root, "controller", error);
    if (controller == nullptr) {
        return false;
    }

    EcuConfig parsed;
    double value = 0.0;
    if (!readNumber(*sensor, "sensor", "startTemp", value, error)) {
        return false;
    }
    parsed.startTemp = static_cast<float>(value);
    if (!readNumber(*sensor, "sensor", "tempStep", value, error)) {
        return false;
    }
    parsed.tempStep = static_cast<float>(value);
    if (!readNumber(*controller, "controller", "warningThreshold", value, error)) {
        return false;
    }
    parsed.warningThreshold = static_cast<float>(value);
    if (!readPeriod(*sensor, "sensor", parsed.sensorPeriod, error)) {
        return false;
    }
    if (!readPeriod(*controller, "controller", parsed.controllerPeriod, error)) {
        return false;
    }

    config = parsed;
    return true;
}
