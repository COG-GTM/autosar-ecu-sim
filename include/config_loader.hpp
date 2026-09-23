//File: include/config_loader.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace config {

// Raised for any configuration problem: unreadable file, invalid JSON,
// missing key or wrong value type.
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message) : std::runtime_error(message) {}
};

inline nlohmann::json load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw ConfigError("Failed to open " + path);
    }
    try {
        return nlohmann::json::parse(file);
    } catch (const nlohmann::json::exception& e) {
        throw ConfigError(path + " is not valid JSON: " + e.what());
    }
}

inline std::vector<std::string> splitPath(const std::string& dottedPath) {
    std::vector<std::string> keys;
    std::istringstream stream(dottedPath);
    std::string key;
    while (std::getline(stream, key, '.')) {
        keys.push_back(key);
    }
    return keys;
}

// Resolves a dotted path such as "sensor.startTemp" and fails with a message
// naming the offending path instead of throwing an unhandled json exception.
inline const nlohmann::json& require(const nlohmann::json& root, const std::string& dottedPath) {
    const nlohmann::json* node = &root;
    std::string walked;
    for (const std::string& key : splitPath(dottedPath)) {
        if (!node->is_object()) {
            throw ConfigError("Config entry '" + walked + "' must be an object");
        }
        if (!node->contains(key)) {
            throw ConfigError("Missing config entry '" + dottedPath + "'");
        }
        node = &node->at(key);
        walked = walked.empty() ? key : walked + "." + key;
    }
    return *node;
}

inline float requireFloat(const nlohmann::json& root, const std::string& dottedPath) {
    const nlohmann::json& value = require(root, dottedPath);
    if (!value.is_number()) {
        throw ConfigError("Config entry '" + dottedPath + "' must be a number");
    }
    return value.get<float>();
}

inline int requireInt(const nlohmann::json& root, const std::string& dottedPath) {
    const nlohmann::json& value = require(root, dottedPath);
    if (!value.is_number_integer()) {
        throw ConfigError("Config entry '" + dottedPath + "' must be an integer");
    }
    return value.get<int>();
}

inline std::string requireString(const nlohmann::json& root, const std::string& dottedPath) {
    const nlohmann::json& value = require(root, dottedPath);
    if (!value.is_string()) {
        throw ConfigError("Config entry '" + dottedPath + "' must be a string");
    }
    return value.get<std::string>();
}

inline const nlohmann::json& requireArray(const nlohmann::json& root, const std::string& dottedPath) {
    const nlohmann::json& value = require(root, dottedPath);
    if (!value.is_array()) {
        throw ConfigError("Config entry '" + dottedPath + "' must be an array");
    }
    return value;
}

}  // namespace config
