#pragma once
#include "speculor/detectors/detector_base.hpp"
#include <vector>
#include <memory>
#include <mutex>

namespace speculor {

struct KDNode {
    std::vector<double> point;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
    size_t axis{0};

    explicit KDNode(const std::vector<double>& p, size_t a = 0)
        : point(p), axis(a) {}
};

class KDTree {
public:
    KDTree() = default;
    ~KDTree() = default;

    void insert(const std::vector<double>& point);
    double nearest_distance(const std::vector<double>& point, size_t k = 3) const;
    void clear();

    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);

private:
    std::unique_ptr<KDNode> root_;
    size_t dimensions_{0};

    std::unique_ptr<KDNode> insert_recursive(std::unique_ptr<KDNode> node,
                                              const std::vector<double>& point,
                                              size_t depth);
    void nearest_recursive(const KDNode* node,
                           const std::vector<double>& point,
                           size_t k, std::vector<double>& best_distances,
                           size_t depth) const;

    double distance(const std::vector<double>& a, const std::vector<double>& b) const;
};

class KDTreeDetector : public Detector {
public:
    explicit KDTreeDetector(size_t max_points = 10000, size_t k_neighbors = 3);

    void update(const Event& event) override;
    double score(const Event& event) const override;
    void reset() override;
    std::vector<uint8_t> serialize() const override;
    void deserialize(const std::vector<uint8_t>& data) override;

private:
    KDTree tree_;
    size_t max_points_{10000};
    size_t k_neighbors_{3};
    mutable std::mutex mutex_;
    mutable std::vector<std::vector<double>> recent_points_;
};

} // namespace speculor