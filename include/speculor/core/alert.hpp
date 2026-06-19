#pragma once
#include <string>
#include <chrono>
#include <cstdint>
#include <optional>

namespace speculor {

enum class Severity {
    Low = 0,
    Medium = 5,
    High = 8,
    Critical = 10
};

struct Alert {
    uint32_t severity{0};          // 0-10 scale
    std::string detector_source;    // which detector flagged it
    std::chrono::system_clock::time_point timestamp;
    double score{0.0};             // anomaly score 0.0 - 1.0
    std::string metric_name;
    double metric_value{0.0};
    std::string description;

    bool is_anomalous() const { return score > 0.7; }
};

} // namespace speculor