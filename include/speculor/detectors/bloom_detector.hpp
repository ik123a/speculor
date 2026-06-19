#pragma once
#include "speculor/detectors/detector_base.hpp"
#include <vector>
#include <cstdint>
#include <array>
#include <functional>

namespace speculor {

class CountingBloomFilter {
public:
    explicit CountingBloomFilter(size_t num_bits = 65536, size_t num_hashes = 4);
    ~CountingBloomFilter() = default;

    void add(const std::vector<uint8_t>& item);
    bool contains(const std::vector<uint8_t>& item) const;
    void remove(const std::vector<uint8_t>& item);
    void decay(double factor);
    void clear();

    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> counters_;
    size_t num_bits_{0};
    size_t num_hashes_{0};

    std::array<size_t, 2> hash(const std::vector<uint8_t>& item) const;
    size_t murmur3(const std::vector<uint8_t>& data, uint32_t seed) const;
};

class BloomDetector : public Detector {
public:
    explicit BloomDetector(size_t num_bits = 65536, size_t num_hashes = 4,
                           double decay_factor = 0.95);

    void update(const Event& event) override;
    double score(const Event& event) const override;
    void reset() override;
    std::vector<uint8_t> serialize() const override;
    void deserialize(const std::vector<uint8_t>& data) override;

private:
    CountingBloomFilter bloom_filter_;
    double decay_factor_{0.95};
    mutable std::mutex mutex_;

    std::vector<uint8_t> event_to_bytes(const Event& event) const;
};

} // namespace speculor