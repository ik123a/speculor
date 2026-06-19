#pragma once
#include "speculor/core/event.hpp"
#include <vector>
#include <cstdint>
#include <memory>

namespace speculor {

class Detector {
public:
    virtual ~Detector() = default;

    /// Update the detector with a new event
    virtual void update(const Event& event) = 0;

    /// Compute anomaly score for the given event (0.0 to 1.0)
    virtual double score(const Event& event) const = 0;

    /// Reset the detector to initial state
    virtual void reset() = 0;

    /// Serialize detector state to binary
    virtual std::vector<uint8_t> serialize() const = 0;

    /// Deserialize detector state from binary
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;
};

} // namespace speculor
