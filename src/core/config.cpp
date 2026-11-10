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
    config.input_queue_size = config_node["input_queue_size"].as<decltype(config.input_queue_size)>(config.input_queue_size);
    config.thread_pool_size = config_node["thread_pool_size"].as<decltype(config.thread_pool_size)>(config.thread_pool_size);
    config.log_level = config_node["log_level"].as<decltype(config.log_level)>(config.log_level);
    config.default_threshold = config_node["default_threshold"].as<decltype(config.default_threshold)>(config.default_threshold);
    config.threshold_window_size = config_node["threshold_window_size"].as<decltype(config.threshold_window_size)>(config.threshold_window_size);
    config.ema_alpha = config_node["ema_alpha"].as<decltype(config.ema_alpha)>(config.ema_alpha);
    config.enable_persistence = config_node["enable_persistence"].as<decltype(config.enable_persistence)>(config.enable_persistence);
    config.storage_path = config_node["storage_path"].as<decltype(config.storage_path)>(config.storage_path);
    config.snapshot_interval_ms = config_node["snapshot_interval_ms"].as<decltype(config.snapshot_interval_ms)>(config.snapshot_interval_ms);

    // Parse detector configurations
    config.detectors.clear();
    auto detectors_node = config_node["detectors"];
    if (detectors_node) {
        for (auto detector : detectors_node) {
            DetectorConfig dc;
            dc.type = detector["type"].as<std::string>("bloom");

            auto params = detector["parameters"];
            for (auto param : params) {
                dc.parameters[param.first.as<std::string>()] = param.second.as<std::string>();
            }

            config.detectors.push_back(dc);
        }
    }

    // Parse plugin paths
    config.plugin_paths.clear();
    auto plugins_node = config_node["plugin_paths"];
    if (plugins_node) {
        for (auto plugin : plugins_node) {
            config.plugin_paths.push_back(plugin.as<std::string>());
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
    config.input_queue_size = config_json.value("input_queue_size", config.input_queue_size);
    config.thread_pool_size = config_json.value("thread_pool_size", config.thread_pool_size);
    config.log_level = config_json.value("log_level", config.log_level);
    config.default_threshold = config_json.value("default_threshold", config.default_threshold);
    config.threshold_window_size = config_json.value("threshold_window_size", config.threshold_window_size);
    config.ema_alpha = config_json.value("ema_alpha", config.ema_alpha);
    config.enable_persistence = config_json.value("enable_persistence", config.enable_persistence);
    config.storage_path = config_json.value("storage_path", config.storage_path);
    config.snapshot_interval_ms = config_json.value("snapshot_interval_ms", config.snapshot_interval_ms);

    // Parse detector configurations
    config.detectors.clear();
    auto detectors_node = config_json.value("detectors", nlohmann::json::array());
    if (!detectors_node.empty()) {
        for (auto detector : detectors_node) {
            DetectorConfig dc;
            dc.type = detector.value("type", std::string("bloom"));

            auto params = detector.value("parameters", nlohmann::json::object());
            for (const auto& param : params.items()) {
                dc.parameters[param.key()] = param.value().get<std::string>();
            }

            config.detectors.push_back(dc);
        }
    }

    // Parse plugin paths
    config.plugin_paths.clear();
    auto plugins_node = config_json.value("plugin_paths", nlohmann::json::array());
    if (!plugins_node.empty()) {
        for (auto plugin : plugins_node) {
            config.plugin_paths.push_back(plugin.get<std::string>());
        }
    }

    return config;
}

std::string Config::to_yaml() const {
    YAML::Node config_node;

    config_node["input_queue_size"] = input_queue_size;
    config_node["thread_pool_size"] = thread_pool_size;
    config_node["log_level"] = log_level;
    config_node["default_threshold"] = default_threshold;
    config_node["threshold_window_size"] = threshold_window_size;
    config_node["ema_alpha"] = ema_alpha;
    config_node["enable_persistence"] = enable_persistence;
    config_node["storage_path"] = storage_path;
    config_node["snapshot_interval_ms"] = snapshot_interval_ms;

    // Add detectors
    YAML::Node detectors_node;
    for (const auto& detector : detectors) {
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
    for (const auto& path : plugin_paths) {
        plugins_node.push_back(path);
    }
    config_node["plugin_paths"] = plugins_node;

    return YAML::Dump(config_node);
}

std::string Config::to_json() const {
    nlohmann::json config_json;

    config_json["input_queue_size"] = input_queue_size;
    config_json["thread_pool_size"] = thread_pool_size;
    config_json["log_level"] = log_level;
    config_json["default_threshold"] = default_threshold;
    config_json["threshold_window_size"] = threshold_window_size;
    config_json["ema_alpha"] = ema_alpha;
    config_json["enable_persistence"] = enable_persistence;
    config_json["storage_path"] = storage_path;
    config_json["snapshot_interval_ms"] = snapshot_interval_ms;

    // Add detectors
    config_json["detectors"] = nlohmann::json::array();
    for (const auto& detector : detectors) {
        config_json["detectors"].push_back(nlohmann::json::object());
        config_json["detectors"].back()["type"] = detector.type;
        for (const auto& param : detector.parameters) {
            config_json["detectors"].back()["parameters"][param.first] = param.second;
        }
    }

    // Add plugin paths
    config_json["plugin_paths"] = nlohmann::json::array();
    for (const auto& path : plugin_paths) {
        config_json["plugin_paths"].push_back(path);
    }

    return config_json.dump(4);  // 4-space indentation
}

} // namespace speculor