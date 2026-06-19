#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <unordered_map>

namespace speculor {

struct Event {
    std::string metric_name;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> labels;

    Event() = default;
    Event(const std::string& name, double val,
          const std::unordered_map<std::string, std::string>& tags = {})
        : metric_name(name), value(val), timestamp(std::chrono::system_clock::now()), labels(tags) {}
};

} // namespace speculor