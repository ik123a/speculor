#pragma once
#include "speculor/detectors/detector_base.hpp"
#include <vector>
#include <memory>
#include <string>

namespace speculor {

class CompositeDetector : public Detector {
public:
    struct DetectorWeight {
        std::shared_ptr<Detector> detector;
        double weight{1.0};
        std::string name;
    };

    CompositeDetector() = default;
    explicit CompositeDetector(const std::vector<DetectorWeight>& detectors);

    void add_detector(const std::shared_ptr<Detector>& detector,
                      double weight = 1.0,
                      const std::string& name = "");

    void update(const Event& event) override;
    double score(const Event& event) const override;
    void reset() override;
    void clear_detectors();
    std::vector<uint8_t> serialize() const override;
    void deserialize(const std::vector<uint8_t>& data) override;

    enum class AggregationMethod {
        WeightedAverage,
        Max,
        WeightedSum
    };

    void set_aggregation_method(AggregationMethod method) { aggregation_method_ = method; }

private:
    std::vector<DetectorWeight> detectors_;
    AggregationMethod aggregation_method_{AggregationMethod::WeightedAverage};
    mutable std::mutex mutex_;
};

} // namespace speculor