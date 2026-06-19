#include "speculor/detectors/composite_detector.hpp"
#include "speculor/core/event.hpp"
#include <numeric>

namespace speculor {

CompositeDetector::CompositeDetector(const std::vector<DetectorWeight>& detectors)
    : detectors_(detectors) {}

void CompositeDetector::add_detector(const std::shared_ptr<Detector>& detector,
                                     double weight,
                                     const std::string& name) {
    detectors_.push_back({detector, weight, name});
}

void CompositeDetector::update(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& dw : detectors_) {
        if (dw.detector) {
            dw.detector->update(event);
        }
    }
}

double CompositeDetector::score(const Event& event) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (detectors_.empty()) return 0.0;
    double result = 0.0;
    switch (aggregation_method_) {
        case AggregationMethod::WeightedAverage: {
            double sum_weights = 0.0;
            for (const auto& dw : detectors_) {
                if (dw.detector) {
                    result += dw.detector->score(event) * dw.weight;
                    sum_weights += dw.weight;
                }
            }
            if (sum_weights > 0.0) result /= sum_weights;
            break;
        }
        case AggregationMethod::Max: {
            result = std::numeric_limits<double>::lowest();
            for (const auto& dw : detectors_) {
                if (dw.detector) {
                    result = std::max(result, dw.detector->score(event));
                }
            }
            break;
        }
        case AggregationMethod::WeightedSum: {
            for (const auto& dw : detectors_) {
                if (dw.detector) {
                    result += dw.detector->score(event) * dw.weight;
                }
            }
            break;
        }
    }
    // Clamp to [0,1]
    if (result < 0.0) result = 0.0;
    if (result > 1.0) result = 1.0;
    return result;
}

void CompositeDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& dw : detectors_) {
        if (dw.detector) dw.detector->reset();
    }
}

void CompositeDetector::clear_detectors() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectors_.clear();
}

std::vector<uint8_t> CompositeDetector::serialize() const {
    std::vector<uint8_t> data;
    // Simple format: number of detectors (size_t) followed by each detector's serialized blob size + blob
    size_t count = detectors_.size();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&count), reinterpret_cast<const uint8_t*>(&count) + sizeof(size_t));
    for (const auto& dw : detectors_) {
        std::vector<uint8_t> det_blob = dw.detector ? dw.detector->serialize() : std::vector<uint8_t>();
        size_t blob_size = det_blob.size();
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&blob_size), reinterpret_cast<const uint8_t*>(&blob_size) + sizeof(size_t));
        data.insert(data.end(), det_blob.begin(), det_blob.end());
        // also serialize weight (double) and name length + name
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&dw.weight), reinterpret_cast<const uint8_t*>(&dw.weight) + sizeof(double));
        size_t name_len = dw.name.size();
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&name_len), reinterpret_cast<const uint8_t*>(&name_len) + sizeof(size_t));
        data.insert(data.end(), dw.name.begin(), dw.name.end());
    }
    return data;
}

void CompositeDetector::deserialize(const std::vector<uint8_t>& data) {
    // For brevity, we provide a stub that clears existing detectors.
    // Full deserialization would require a registry of detector factories.
    detectors_.clear();
    // No further action – user must rebuild the composite manually.
}

} // namespace speculor