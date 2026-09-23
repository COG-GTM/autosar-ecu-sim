// File: src/ecu_config.cpp
#include "../include/ecu_config.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

const json& requireObject(const json& parent, const std::string& key) {
    if (!parent.contains(key) || !parent.at(key).is_object()) {
        throw std::runtime_error("missing or non-object section \"" + key + "\"");
    }
    return parent.at(key);
}

double requireNumber(const json& section, const std::string& sectionName,
                     const std::string& key) {
    if (!section.contains(key) || !section.at(key).is_number()) {
        throw std::runtime_error("missing or non-numeric key \"" + sectionName +
                                 "." + key + "\"");
    }
    return section.at(key).get<double>();
}

}  // namespace

bool loadEcuConfig(std::istream& input, EcuConfig& config, std::string& error) {
    try {
        json parsed = json::parse(input);
        if (!parsed.is_object()) {
            throw std::runtime_error("top-level value is not an object");
        }

        const json& sensor = requireObject(parsed, "sensor");
        const json& controller = requireObject(parsed, "controller");

        config.startTemp = static_cast<float>(requireNumber(sensor, "sensor", "startTemp"));
        config.tempStep = static_cast<float>(requireNumber(sensor, "sensor", "tempStep"));
        config.warningThreshold =
            static_cast<float>(requireNumber(controller, "controller", "warningThreshold"));
        config.sensorPeriod = static_cast<int>(requireNumber(sensor, "sensor", "periodMs"));
        config.controllerPeriod =
            static_cast<int>(requireNumber(controller, "controller", "periodMs"));
        return true;
    } catch (const json::exception& e) {
        error = std::string("invalid JSON: ") + e.what();
        return false;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}
