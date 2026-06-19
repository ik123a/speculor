#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace speculor {

struct DetectorConfig {
    std::string type;  // "bloom", "cms", "kdtree", "composite"
    std::unordered_map<std::string, std::string> parameters;
};

struct Config {
    // Engine settings
    size_t input_queue_size{1024};
    size_t thread_pool_size{4};
    std::string log_level{"info"};

    // Threshold settings
    double default_threshold{0.75};
    size_t threshold_window_size{1000};
    double ema_alpha{0.3};

    // Storage settings
    bool enable_persistence{false};
    std::string storage_path{"./SPECULOR_data"};
    size_t snapshot_interval_ms{60000};

    // Detector configurations
    std::vector<DetectorConfig> detectors;

    // Plugin settings
    std::vector<std::string> plugin_paths;

    static Config from_yaml(const std::string& path);
    static Config from_json(const std::string& path);
    std::string to_yaml() const;
    std::string to_json() const;
};

} // namespace speculor