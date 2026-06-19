#include "speculor/detectors/bloom_detector.hpp"
#include <cstdint>
#include <random>
#include <mutex>
#include <vector>
#include "speculor/core/event.hpp"

namespace speculor {

CountingBloomFilter::CountingBloomFilter(size_t num_bits, size_t num_hashes)
    : num_bits_(num_bits), num_hashes_(num_hashes) {
    counters_.resize(num_bits, 0);
}

void CountingBloomFilter::add(const std::vector<uint8_t>& item) {
    for (size_t i = 0; i < num_hashes_; ++i) {
        size_t hash_value = murmur3(item, i);
        size_t index = hash_value % num_bits_;
        counters_[index]++;
    }
}

bool CountingBloomFilter::contains(const std::vector<uint8_t>& item) const {
    for (size_t i = 0; i < num_hashes_; ++i) {
        size_t hash_value = murmur3(item, i);
        size_t index = hash_value % num_bits_;
        if (counters_[index] == 0) {
            return false;
        }
    }
    return true;
}

void CountingBloomFilter::remove(const std::vector<uint8_t>& item) {
    for (size_t i = 0; i < num_hashes_; ++i) {
        size_t hash_value = murmur3(item, i);
        size_t index = hash_value % num_bits_;
        if (counters_[index] > 0) {
            counters_[index]--;
        }
    }
}

void CountingBloomFilter::decay(double factor) {
    for (size_t i = 0; i < num_bits_; ++i) {
        counters_[i] = static_cast<size_t>(counters_[i] * factor);
    }
}

void CountingBloomFilter::clear() {
    std::fill(counters_.begin(), counters_.end(), 0);
}

std::vector<uint8_t> CountingBloomFilter::serialize() const {
    return counters_;
}

void CountingBloomFilter::deserialize(const std::vector<uint8_t>& data) {
    counters_ = data;
}

std::array<size_t, 2> CountingBloomFilter::hash(const std::vector<uint8_t>& item) const {
    std::array<size_t, 2> result;
    result[0] = murmur3(item, 0);
    result[1] = murmur3(item, 1);
    return result;
}

size_t CountingBloomFilter::murmur3(const std::vector<uint8_t>& data, uint32_t seed) const {
    // Simple implementation of MurmurHash3
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h = seed ^ data.size();

    for (size_t i = 0; i < data.size(); i++) {
        uint32_t k = data[i];
        k *= m;
        k &= 0xFFFFFFFF;
        k *= m;
        k ^= h;
        h = k >> r | k << (32 - r);
    }

    return static_cast<size_t>(h ^ (h >> 13)) >> 15;
}

std::vector<uint8_t> BloomDetector::event_to_bytes(const Event& event) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(event.metric_name.size() + sizeof(double) + 1);

    // Copy metric name
    bytes.insert(bytes.end(), event.metric_name.begin(), event.metric_name.end());
    bytes.push_back(0);  // Null terminator

    // Copy double value (8 bytes)
    double val = event.value;
    uint8_t val_bytes[sizeof(double)];
    std::memcpy(val_bytes, &val, sizeof(double));
    bytes.insert(bytes.end(), val_bytes, val_bytes + sizeof(double));

    return bytes;
}

BloomDetector::BloomDetector(size_t num_bits, size_t num_hashes, double decay_factor)
    : bloom_filter_(num_bits, num_hashes), decay_factor_(decay_factor) {}

void BloomDetector::update(const Event& event) {
    std::vector<uint8_t> bytes = event_to_bytes(event);
    std::lock_guard<std::mutex> lock(mutex_);
    bloom_filter_.add(bytes);
}

double BloomDetector::score(const Event& event) const {
    std::vector<uint8_t> bytes = event_to_bytes(event);
    std::lock_guard<std::mutex> lock(mutex_);
    bool present = bloom_filter_.contains(bytes);
    // For this implementation, we'll use a simple scoring approach
    // High score if not present (new pattern), low score if present
    return present ? 0.1 : 0.8;
}

void BloomDetector::reset() {
    bloom_filter_.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    // Reset any state if needed
}

std::vector<uint8_t> BloomDetector::serialize() const {
    return bloom_filter_.serialize();
}

void BloomDetector::deserialize(const std::vector<uint8_t>& data) {
    bloom_filter_.deserialize(data);
}

} // namespace speculor