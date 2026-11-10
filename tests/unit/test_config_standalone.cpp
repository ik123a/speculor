// Standalone configuration contract: links yaml-cpp, no engine/storage/gtest.
#include "speculor/core/config.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void equal(const speculor::Config& a, const speculor::Config& b) {
    require(a.input_queue_size == b.input_queue_size, "queue size");
    require(a.thread_pool_size == b.thread_pool_size, "thread count");
    require(a.log_level == b.log_level, "log level");
    require(a.default_threshold == b.default_threshold, "threshold");
    require(a.threshold_window_size == b.threshold_window_size, "window");
    require(a.ema_alpha == b.ema_alpha, "alpha");
    require(a.enable_persistence == b.enable_persistence, "persistence flag");
    require(a.storage_path == b.storage_path, "storage path");
    require(a.snapshot_interval_ms == b.snapshot_interval_ms, "snapshot interval");
    require(a.plugin_paths == b.plugin_paths, "plugin paths");
    require(a.detectors.size() == b.detectors.size(), "detector count");
    for (size_t i = 0; i < a.detectors.size(); ++i) {
        require(a.detectors[i].type == b.detectors[i].type, "detector type");
        require(a.detectors[i].parameters == b.detectors[i].parameters, "parameters");
    }
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;  // Writes only to an explicitly supplied fixture directory.
    try {
        const auto root = std::filesystem::path(argv[1]);
        std::filesystem::create_directories(root);
        const auto json = root / "config.json";
        const auto yaml = root / "config.yaml";
        { std::ofstream(json) << "{}"; std::ofstream(yaml) << "{}"; }
        equal(speculor::Config{}, speculor::Config::from_json(json.string()));
        equal(speculor::Config{}, speculor::Config::from_yaml(yaml.string()));

        speculor::Config config;
        config.input_queue_size = 4096;
        config.thread_pool_size = 8;
        config.log_level = "debug";
        config.default_threshold = 0.0;
        config.threshold_window_size = 0;
        config.ema_alpha = 0.0;
        config.enable_persistence = true;
        config.storage_path = "./fixture-data";
        config.snapshot_interval_ms = 0;
        config.detectors = {{"cms", {{"width", "2048"}, {"depth", "5"}}}, {"bloom", {}}};
        config.plugin_paths = {"fixture/plugin.dll", "other/plugin.so"};
        for (bool persistence : {true, false}) {
            config.enable_persistence = persistence;
            { std::ofstream(json) << config.to_json(); std::ofstream(yaml) << config.to_yaml(); }
            equal(config, speculor::Config::from_json(json.string()));
            equal(config, speculor::Config::from_yaml(yaml.string()));
        }
        for (const auto& path : {json, yaml}) {
            std::ofstream(path) << "{";
            bool rejected = false;
            try {
                if (path == json) speculor::Config::from_json(path.string());
                else speculor::Config::from_yaml(path.string());
            } catch (const std::runtime_error&) { rejected = true; }
            require(rejected, "malformed input must throw");
        }
        std::cout << "PASS: JSON/YAML defaults, typed round trips and malformed input\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
