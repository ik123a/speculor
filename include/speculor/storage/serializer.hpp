#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace speculor {

// Forward declarations for FlatBuffers generated code
// Will be generated from flatbuffers schema

class Serializer {
public:
    Serializer();
    ~Serializer();

    // Serialize various state objects to bytes
    std::vector<uint8_t> serialize_bloom(const class CountingBloomFilter& bloom);
    std::vector<uint8_t> serialize_cms(const class CountMinSketch& cms);
    std::vector<uint8_t> serialize_kdtree(const class KDTree& tree);
    std::vector<uint8_t> serialize_thresholds(const class ThresholdEngine& engine);

    // Deserialize from bytes
    bool deserialize_bloom(const std::vector<uint8_t>& data, class CountingBloomFilter& bloom);
    bool deserialize_cms(const std::vector<uint8_t>& data, class CountMinSketch& cms);
    bool deserialize_kdtree(const std::vector<uint8_t>& data, class KDTree& tree);
    bool deserialize_thresholds(const std::vector<uint8_t>& data, class ThresholdEngine& engine);

    // Full snapshot
    std::vector<uint8_t> create_snapshot(const class SpeculorEngine& engine);
    bool apply_snapshot(class SpeculorEngine& engine, const std::vector<uint8_t>& data);
};

} // namespace speculor