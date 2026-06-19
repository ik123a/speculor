#include "speculor/detectors/cms_detector.hpp"
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include "speculor/core/event.hpp"
#include <algorithm>

namespace speculor {

CountMinSketch::CountMinSketch(size_t width, size_t depth)
    : width_(width), depth_(depth) {
    table_.resize(depth, std::vector<uint32_t>(width, 0));
}

void CountMinSketch::increment(const std::vector<uint8_t>& item) {
    for (size_t i = 0; i < depth_; ++i) {
        uint32_t hash_val = hash(item, i);
        size_t idx = hash_val % width_;
        table_[i][idx] += 1;
    }
}

uint32_t CountMinSketch::estimate(const std::vector<uint8_t>& item) const {
    uint32_t min_val = UINT32_MAX;
    for (size_t i = 0; i < depth_; ++i) {
        uint32_t hash_val = hash(item, i);
        size_t idx = hash_val % width_;
        min_val = std::min(min_val, table_[i][idx]);
    }
    return min_val;
}

void CountMinSketch::decay(double factor) {
    for (size_t i = 0; i < depth_; ++i) {
        for (size_t j = 0; j < width_; ++j) {
            table_[i][j] = static_cast<uint32_t>(table_[i][j] * factor);
        }
    }
}

void CountMinSketch::clear() {
    std::fill(table_.begin(), table_.end(), std::vector<uint32_t>(width_, 0));
}

std::vector<uint8_t> CountMinSketch::serialize() const {
    std::vector<uint8_t> data;
    // Serialize width and depth
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&width_), sizeof(size_t));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&depth_), sizeof(size_t));
    // Serialize table
    for (size_t i = 0; i < depth_; ++i) {
        for (size_t j = 0; j < width_; ++j) {
            uint32_t val = table_[i][j];
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&val), sizeof(uint32_t));
        }
    }
    return data;
}

void CountMinSketch::deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    // Read width
    size_t w;
    std::memcpy(&w, data.data() + pos, sizeof(size_t));
    pos += sizeof(size_t);
    // Read depth
    size_t d;
    std::memcpy(&d, data.data() + pos, sizeof(size_t));
    pos += sizeof(size_t);
    // Resize table
    table_.resize(d, std::vector<uint32_t>(w, 0));
    // Read table values
    for (size_t i = 0; i < d; ++i) {
        for (size_t j = 0; j < w; ++j) {
            std::memcpy(&table_[i][j], data.data() + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);
        }
    }
}

size_t CountMinSketch::hash(const std::vector<uint8_t>& item, size_t seed) const {
    // Simple hash function
    uint32_t h = seed ^ item.size();
    for (size_t i = 0; i < item.size(); ++i) {
        h ^= item[i];
        h = (h * 2654435761u) ^ (h >> 16);
    }
    return h;
}

std::vector<uint8_t> CMSDetector::event_to_bytes(const Event& event) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(event.metric_name.size() + sizeof(double) + 1);
    bytes.insert(bytes.end(), event.metric_name.begin(), event.metric_name.end());
    bytes.push_back(0);
    double val = event.value;
    uint8_t val_bytes[sizeof(double)];
    std::memcpy(val_bytes, &val, sizeof(double));
    bytes.insert(bytes.end(), val_bytes, val_bytes + sizeof(double));
    return bytes;
}

CMSDetector::CMSDetector(size_t width, size_t depth, double decay_factor)
    : sketch_(width, depth), decay_factor_(decay_factor) {
    // Initialize global average frequency estimator
    global_average_.store(1.0);
}

void CMSDetector::update(const Event& event) {
    std::vector<uint8_t> bytes = event_to_bytes(event);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sketch_.increment(bytes);
        // Update global average - this is simplified
        double count = 1.0;  // Each event counts as 1
        double current_avg = global_average_.load(std::memory_order_relaxed);
        global_average_.store((count + current_avg) / (2.0));  // Simple averaging
    }
}

double CMSDetector::score(const Event& event) const {
    std::vector<uint8_t> bytes = event_to_bytes(event);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t est_count = sketch_.estimate(bytes);
        double current_avg = global_average_.load(std::memory_order_relaxed);
        // Score based on rarity - lower count means more anomalous
        // We'll use a simple formula: score = min(1.0, est_count / (current_avg + 1))
        return std::min(1.0, est_count / (current_avg + 1.0));
    }
}

void CMSDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    sketch_.clear();
    global_average_.store(1.0);
}

std::vector<uint8_t> CMSDetector::serialize() const {
    return sketch_.serialize();
}

void CMSDetector::deserialize(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    sketch_.deserialize(data);
    // Reset global_average to default
    global_average_.store(1.0);
}

} // namespace speculor