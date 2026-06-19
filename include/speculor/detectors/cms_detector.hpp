#pragma once
#include "speculor/detectors/detector_base.hpp"
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>

namespace speculor {

class CountMinSketch {
public:
    CountMinSketch(size_t width = 1024, size_t depth = 4);
    ~CountMinSketch() = default;

    void increment(const std::vector<uint8_t>& item);
    uint32_t estimate(const std::vector<uint8_t>& item) const;
    void decay(double factor);
    void clear();

    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);

private:
    std::vector<std::vector<uint32_t>> table_;
    size_t width_{0};
    size_t depth_{0};

    size_t hash(const std::vector<uint8_t>& item, size_t seed) const;
};

class CMSDetector : public Detector {
public:
    explicit CMSDetector(size_t width = 1024, size_t depth = 4,
                         double decay_factor = 0.95);

    void update(const Event& event) override;
    double score(const Event& event) const override;
    void reset() override;
    std::vector<uint8_t> serialize() const override;
    void deserialize(const std::vector<uint8_t>& data) override;

private:
    CountMinSketch sketch_;
    double decay_factor_{0.95};
    mutable std::mutex mutex_;
    mutable std::atomic<double> global_average_{1.0};

    std::vector<uint8_t> event_to_bytes(const Event& event) const;
};

} // namespace speculor