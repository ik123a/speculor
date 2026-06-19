#include "speculor/core/config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace speculor {

Config Config::from_yaml(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open YAML file: " + path);
    }

    YAML::Node config_node;
    try {
        config_node = YAML::Load(file);
    } catch (const YAML::ParserException& e) {
        throw std::runtime_error("YAML parse error in " + path + ": " + e.what());
    }

    Config config;

    // Read configuration values with defaults
    config.input_queue_size = config_node["input_queue_size"] || 1024;
    config.thread_pool_size = config_node["thread_pool_size"] || 4;
    config.log_level = config_node["log_level"] || "info";
    config.default_threshold = config_node["default_threshold"] || 0.75;
    config.threshold_window_size = config_node["threshold_window_size"] || 1000;
    config.ema_alpha = config_node["ema_alpha"] || 0.3;
    config.enable_persistence = config_node["enable_persistence"] || false;
    config.storage_path = config_node["storage_path"] || "./SPECULOR_data";
    config.snapshot_interval_ms = config_node["snapshot_interval_ms"] || 60000;

    // Parse detector configurations
    config.detectors.clear();
    auto detectors_node = config_node["detectors"];
    if (detectors_node) {
        for (auto detector : detectors_node) {
            DetectorConfig dc;
            dc.type = detector["type"] || "bloom";

            auto params = detector["parameters"];
            for (auto param : params) {
                dc.parameters[param.first.asString()] = param.second.asString();
            }

            config.detectors.push_back(dc);
        }
    }

    // Parse plugin paths
    config.plugin_paths.clear();
    auto plugins_node = config_node["plugin_paths"];
    if (plugins_node) {
        for (auto plugin : plugins_node) {
            config.plugin_paths.push_back(plugin.asString());
        }
    }

    return config;
}

Config Config::from_json(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    nlohmann::json config_json;
    try {
        file >> config_json;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + path + ": " + e.what());
    }

    Config config;

    // Read configuration values with defaults
    config.input_queue_size = config_json["input_queue_size"] || 1024;
    config.thread_pool_size = config_json["thread_pool_size"] || 4;
    config.log_level = config_json["log_level"] || "info";
    config.default_threshold = config_json["default_threshold"] || 0.75;
    config.threshold_window_size = config_json["threshold_window_size"] || 1000;
    config.ema_alpha = config_json["ema_alpha"] || 0.3;
    config.enable_persistence = config_json["enable_persistence"] || false;
    config.storage_path = config_json["storage_path"] || "./SPECULOR_data";
    config.snapshot_interval_ms = config_json["snapshot_interval_ms"] || 60000;

    // Parse detector configurations
    config.detectors.clear();
    auto detectors_node = config_json["detectors"];
    if (detectors_node) {
        for (auto detector : detectors_node) {
            DetectorConfig dc;
            dc.type = detector["type"] || "bloom";

            auto params = detector["parameters"];
            for (auto param : params) {
                dc.parameters[param.first.get<std::string>() ] = param.second.get<std::string>();
            }

            config.detectors.push_back(dc);
        }
    }

    // Parse plugin paths
    config.plugin_paths.clear();
    auto plugins_node = config_json["plugin_paths"];
    if (plugins_node) {
        for (auto plugin : plugins_node) {
            config.plugin_paths.push_back(plugin.get<std::string>());
        }
    }

    return config;
}

std::string Config::to_yaml() const {
    YAML::Node config_node;

    config_node["input_queue_size"] = config_.input_queue_size;
    config_node["thread_pool_size"] = config_.thread_pool_size;
    config_node["log_level"] = config_.log_level;
    config_node["default_threshold"] = config_.default_threshold;
    config_node["threshold_window_size"] = config_.threshold_window_size;
    config_node["ema_alpha"] = config_.ema_alpha;
    config_node["enable_persistence"] = config_.enable_persistence;
    config_node["storage_path"] = config_.storage_path;
    config_node["snapshot_interval_ms"] = config_.snapshot_interval_ms;

    // Add detectors
    YAML::Node detectors_node;
    for (const auto& detector : config_.detectors) {
        YAML::Node detector_node;
        detector_node["type"] = detector.type;
        for (const auto& param : detector.parameters) {
            detector_node["parameters"][param.first] = param.second;
        }
        detectors_node.push_back(detector_node);
    }
    config_node["detectors"] = detectors_node;

    // Add plugin paths
    YAML::Node plugins_node;
    for (const auto& path : config_.plugin_paths) {
        plugins_node.push_back(path);
    }
    config_node["plugin_paths"] = plugins_node;

    return YAML::Dump(config_node);
}

std::string Config::to_json() const {
    nlohmann::json config_json;

    config_json["input_queue_size"] = config_.input_queue_size;
    config_json["thread_pool_size"] = config_.thread_pool_size;
    config_json["log_level"] = config_.log_level;
    config_json["default_threshold"] = config_.default_threshold;
    config_json["threshold_window_size"] = config_.threshold_window_size;
    config_json["ema_alpha"] = config_.ema_alpha;
    config_json["enable_persistence"] = config_.enable_persistence;
    config_json["storage_path"] = config_.storage_path;
    config_json["snapshot_interval_ms"] = config_.snapshot_interval_ms;

    // Add detectors
    config_json["detectors"] = nlohmann::json::array();
    for (const auto& detector : config_.detectors) {
        config_json["detectors"].push_back(nlohmann::json::object());
        config_json["detectors"].back()["type"] = detector.type;
        for (const auto& param : detector.parameters) {
            config_json["detectors"].back()["parameters"][param.first] = param.second;
        }
    }

    // Add plugin paths
    config_json["plugin_paths"] = nlohmann::json::array();
    for (const auto& path : config_.plugin_paths) {
        config_json["plugin_paths"].push_back(path);
    }

    return config_json.dump(4);  // 4-space indentation
}

} // namespace speculor