#include "speculor/detectors/kdtree_detector.hpp"
#include <algorithm>
#include <mutex>
#include <cmath>
#include <cstring>
#include "speculor/core/event.hpp"

namespace speculor {

// --- KDTree Implementation ---

double KDTree::distance(const std::vector<double>& a, const std::vector<double>& b) const {
    double sum = 0.0;
    size_t dims = std::min(a.size(), b.size());
    for (size_t i = 0; i < dims; ++i) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

void KDTree::insert(const std::vector<double>& point) {
    if (point.empty()) return;
    if (dimensions_ == 0) {
        dimensions_ = point.size();
    }
    root_ = insert_recursive(std::move(root_), point, 0);
}

std::unique_ptr<KDNode> KDTree::insert_recursive(std::unique_ptr<KDNode> node,
                                                  const std::vector<double>& point,
                                                  size_t depth) {
    if (!node) {
        return std::make_unique<KDNode>(point, depth % dimensions_);
    }
    size_t axis = node->axis;
    if (point[axis] < node->point[axis]) {
        node->left = insert_recursive(std::move(node->left), point, depth + 1);
    } else {
        node->right = insert_recursive(std::move(node->right), point, depth + 1);
    }
    return node;
}

double KDTree::nearest_distance(const std::vector<double>& point, size_t k) const {
    if (!root_ || point.empty()) return 0.0;
    std::vector<double> best_distances(k, 1e9);
    nearest_recursive(root_.get(), point, k, best_distances, 0);
    
    double sum = 0.0;
    size_t count = 0;
    for (double d : best_distances) {
        if (d < 1e9) {
            sum += d;
            count++;
        }
    }
    return count > 0 ? (sum / count) : 0.0;
}

void KDTree::nearest_recursive(const KDNode* node,
                               const std::vector<double>& point,
                               size_t k, std::vector<double>& best_distances,
                               size_t depth) const {
    if (!node) return;

    double d = distance(point, node->point);
    if (d < best_distances.back()) {
        best_distances.back() = d;
        std::sort(best_distances.begin(), best_distances.end());
    }

    size_t axis = node->axis;
    const KDNode* next_node = nullptr;
    const KDNode* other_node = nullptr;

    if (point[axis] < node->point[axis]) {
        next_node = node->left.get();
        other_node = node->right.get();
    } else {
        next_node = node->right.get();
        other_node = node->left.get();
    }

    nearest_recursive(next_node, point, k, best_distances, depth + 1);

    double plane_dist = std::abs(point[axis] - node->point[axis]);
    if (plane_dist < best_distances.back()) {
        nearest_recursive(other_node, point, k, best_distances, depth + 1);
    }
}

void KDTree::clear() {
    root_.reset();
    dimensions_ = 0;
}

std::vector<uint8_t> KDTree::serialize() const {
    std::vector<uint8_t> data;
    auto serialize_node = [&](auto& self, const KDNode* node) -> void {
        if (!node) {
            uint8_t flag = 0;
            data.push_back(flag);
            return;
        }
        uint8_t flag = 1;
        data.push_back(flag);
        uint64_t size = node->point.size();
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(uint64_t));
        for (double val : node->point) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&val), reinterpret_cast<const uint8_t*>(&val) + sizeof(double));
        }
        uint64_t axis = node->axis;
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&axis), reinterpret_cast<const uint8_t*>(&axis) + sizeof(uint64_t));
        self(self, node->left.get());
        self(self, node->right.get());
    };
    uint64_t dims = dimensions_;
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&dims), reinterpret_cast<const uint8_t*>(&dims) + sizeof(uint64_t));
    serialize_node(serialize_node, root_.get());
    return data;
}

void KDTree::deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    size_t pos = 0;
    uint64_t dims;
    std::memcpy(&dims, data.data() + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    dimensions_ = dims;

    auto deserialize_node = [&](auto& self) -> std::unique_ptr<KDNode> {
        if (pos >= data.size()) return nullptr;
        uint8_t flag = data[pos++];
        if (flag == 0) return nullptr;

        uint64_t size;
        std::memcpy(&size, data.data() + pos, sizeof(uint64_t));
        pos += sizeof(uint64_t);
        std::vector<double> point(size);
        for (size_t i = 0; i < size; ++i) {
            std::memcpy(&point[i], data.data() + pos, sizeof(double));
            pos += sizeof(double);
        }
        uint64_t axis;
        std::memcpy(&axis, data.data() + pos, sizeof(uint64_t));
        pos += sizeof(uint64_t);

        auto node = std::make_unique<KDNode>(point, axis);
        node->left = self(self);
        node->right = self(self);
        return node;
    };
    root_ = deserialize_node(deserialize_node);
}

// --- KDTreeDetector Implementation ---

KDTreeDetector::KDTreeDetector(size_t max_points, size_t k_neighbors)
    : max_points_(max_points), k_neighbors_(k_neighbors) {
}

void KDTreeDetector::update(const Event& event) {
    std::vector<double> features;
    features.push_back(static_cast<double>(event.metric_name.length()));
    features.push_back(event.value);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (recent_points_.size() >= max_points_) {
            recent_points_.erase(recent_points_.begin());
        }
        recent_points_.push_back(features);
    }

    tree_.insert(features);
}

double KDTreeDetector::score(const Event& event) const {
    std::vector<double> features;
    features.push_back(static_cast<double>(event.metric_name.length()));
    features.push_back(event.value);

    std::lock_guard<std::mutex> lock(mutex_);
    if (recent_points_.empty()) return 0.0;

    double min_distance = tree_.nearest_distance(features, k_neighbors_);

    double normalized_distance = std::min(1.0, min_distance / 100.0);

    return 1.0 - normalized_distance;
}

void KDTreeDetector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tree_.clear();
    recent_points_.clear();
}

std::vector<uint8_t> KDTreeDetector::serialize() const {
    std::vector<uint8_t> data;

    uint64_t mp = max_points_;
    uint64_t kn = k_neighbors_;
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&mp), sizeof(uint64_t));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&kn), sizeof(uint64_t));

    // Serialize tree
    std::vector<uint8_t> tree_data = tree_.serialize();
    uint64_t tree_size = tree_data.size();
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&tree_size), sizeof(uint64_t));
    data.insert(data.end(), tree_data.begin(), tree_data.end());

    // Serialize recent points
    uint64_t size = recent_points_.size();
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&size), sizeof(uint64_t));
    for (const auto& point : recent_points_) {
        uint64_t pt_size = point.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&pt_size), sizeof(uint64_t));
        for (double val : point) {
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&val), sizeof(double));
        }
    }

    return data;
}

void KDTreeDetector::deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    size_t pos = 0;
    
    uint64_t mp, kn;
    std::memcpy(&mp, data.data() + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    std::memcpy(&kn, data.data() + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);

    max_points_ = mp;
    k_neighbors_ = kn;

    tree_.clear();
    recent_points_.clear();

    // Read tree data
    uint64_t tree_size;
    std::memcpy(&tree_size, data.data() + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    std::vector<uint8_t> tree_data(data.begin() + pos, data.begin() + pos + tree_size);
    tree_.deserialize(tree_data);
    pos += tree_size;

    // Read recent points count
    uint64_t point_count;
    std::memcpy(&point_count, data.data() + pos, sizeof(uint64_t));
    pos += sizeof(uint64_t);

    recent_points_.resize(point_count);
    for (uint64_t i = 0; i < point_count; ++i) {
        uint64_t pt_size;
        std::memcpy(&pt_size, data.data() + pos, sizeof(uint64_t));
        pos += sizeof(uint64_t);
        recent_points_[i].resize(pt_size);
        for (uint64_t j = 0; j < pt_size; ++j) {
            std::memcpy(&recent_points_[i][j], data.data() + pos, sizeof(double));
            pos += sizeof(double);
        }
    }
}

} // namespace speculor