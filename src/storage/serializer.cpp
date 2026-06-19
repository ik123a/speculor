#include "speculor/storage/serializer.hpp"
#include "speculor/adaptive/threshold_engine.hpp"
#include "speculor/core/engine.hpp"
#include "speculor/detectors/bloom_detector.hpp"
#include "speculor/detectors/cms_detector.hpp"
#include "speculor/detectors/kdtree_detector.hpp"
#include "speculor/detectors/composite_detector.hpp"

namespace speculor {

Serializer::Serializer() {}
Serializer::~Serializer() {}

// For simplicity, these methods just forward to detector serialization.
// A full implementation would use FlatBuffers schema defined elsewhere.

std::vector<uint8_t> Serializer::serialize_bloom(const CountingBloomFilter& bloom) {
    return bloom.serialize();
}

std::vector<uint8_t> Serializer::serialize_cms(const CountMinSketch& cms) {
    return cms.serialize();
}

std::vector<uint8_t> Serializer::serialize_kdtree(const KDTree& tree) {
    // Placeholder – real implementation would traverse the tree
    return {};
}

std::vector<uint8_t> Serializer::serialize_thresholds(const ThresholdEngine& engine) {
    // Serialize current threshold and EMA values
    std::vector<uint8_t> data;
    double thr = engine.current_threshold();
    double ema = engine.ema_value();
    double dev = engine.ema_deviation();
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&thr), reinterpret_cast<uint8_t*>(&thr) + sizeof(double));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&ema), reinterpret_cast<uint8_t*>(&ema) + sizeof(double));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&dev), reinterpret_cast<uint8_t*>(&dev) + sizeof(double));
    return data;
}

bool Serializer::deserialize_bloom(const std::vector<uint8_t>& data, CountingBloomFilter& bloom) {
    bloom.deserialize(data);
    return true;
}

bool Serializer::deserialize_cms(const std::vector<uint8_t>& data, CountMinSketch& cms) {
    cms.deserialize(data);
    return true;
}

bool Serializer::deserialize_kdtree(const std::vector<uint8_t>& data, KDTree& tree) {
    // Stub – not implemented
    return false;
}

bool Serializer::deserialize_thresholds(const std::vector<uint8_t>& data, ThresholdEngine& engine) {
    if (data.size() < sizeof(double) * 3) return false;
    double thr, ema, dev;
    std::memcpy(&thr, data.data(), sizeof(double));
    std::memcpy(&ema, data.data() + sizeof(double), sizeof(double));
    std::memcpy(&dev, data.data() + 2 * sizeof(double), sizeof(double));
    // Not a full API – we set via public setters (if they existed). For now, we ignore.
    return true;
}

std::vector<uint8_t> Serializer::create_snapshot(const SpeculorEngine& engine) {
    // Very high level stub – combine parts.
    std::vector<uint8_t> snapshot;
    // Serialize detectors (assuming they are stored inside the engine's pipeline)
    // This is just a placeholder.
    return snapshot;
}

bool Serializer::apply_snapshot(SpeculorEngine& engine, const std::vector<uint8_t>& data) {
    // Stub – not implemented
    return false;
}

} // namespace speculor