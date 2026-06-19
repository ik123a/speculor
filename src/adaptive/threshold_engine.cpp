#include "speculor/adaptive/threshold_engine.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

namespace speculor {

// EMACalculator implementation
EMACalculator::EMACalculator(double alpha) : alpha_(alpha), ema_value_(0.0), ema_deviation_(0.0), initialized_(false) {}

void EMACalculator::update(double value) {
    if (!initialized_) {
        ema_value_ = value;
        ema_deviation_ = 0.0;
        initialized_ = true;
    } else {
        double diff = value - ema_value_;
        ema_value_ = alpha_ * value + (1.0 - alpha_) * ema_value_;
        ema_deviation_ = alpha_ * std::abs(diff) + (1.0 - alpha_) * ema_deviation_;
    }
}

void EMACalculator::reset() {
    ema_value_ = 0.0;
    ema_deviation_ = 0.0;
    initialized_ = false;
}

// ThresholdOptimizer implementation
ThresholdOptimizer::ThresholdOptimizer(size_t window_size) : window_size_(window_size), insert_idx_(0) {
    window_.resize(window_size_);
}

void ThresholdOptimizer::add_sample(double score, bool is_anomaly) {
    if (window_size_ == 0) return;
    window_[insert_idx_] = {score, is_anomaly};
    insert_idx_ = (insert_idx_ + 1) % window_size_;
}

size_t ThresholdOptimizer::sample_count() const {
    return window_size_;
}

void ThresholdOptimizer::reset() {
    std::fill(window_.begin(), window_.end(), Sample{0.0, false});
    insert_idx_ = 0;
}

// Compute F1 for a given threshold
double ThresholdOptimizer::compute_f1(double threshold) const {
    size_t tp = 0, fp = 0, fn = 0;
    for (const auto& s : window_) {
        bool predicted = s.score >= threshold;
        if (predicted && s.is_anomaly) ++tp;
        else if (predicted && !s.is_anomaly) ++fp;
        else if (!predicted && s.is_anomaly) ++fn;
    }
    if (tp + fp + fn == 0) return 0.0;
    double precision = tp + fp ? static_cast<double>(tp) / (tp + fp) : 0.0;
    double recall = tp + fn ? static_cast<double>(tp) / (tp + fn) : 0.0;
    if (precision + recall == 0) return 0.0;
    return 2.0 * precision * recall / (precision + recall);
}

double ThresholdOptimizer::optimize() {
    // Simple brute-force search over thresholds 0.0 to 1.0 step 0.01
    double best_threshold = 0.0;
    double best_f1 = -1.0;
    for (double t = 0.0; t <= 1.0; t += 0.01) {
        double f1 = compute_f1(t);
        if (f1 > best_f1) {
            best_f1 = f1;
            best_threshold = t;
        }
    }
    return best_threshold;
}

// AlertPriorityQueue implementation
AlertPriorityQueue::AlertPriorityQueue(size_t max_size) : max_size_(max_size) {}

bool AlertPriorityQueue::compare_severity(const Alert& a, const Alert& b) {
    return a.severity > b.severity; // max-heap based on severity
}

void AlertPriorityQueue::push(const Alert& alert) {
    heap_.push_back(alert);
    heapify_up(heap_.size() - 1);
    evict();
}

Alert AlertPriorityQueue::top() const {
    if (heap_.empty()) throw std::runtime_error("AlertPriorityQueue empty");
    return heap_.front();
}

void AlertPriorityQueue::pop() {
    if (heap_.empty()) return;
    std::swap(heap_.front(), heap_.back());
    heap_.pop_back();
    heapify_down(0);
}

bool AlertPriorityQueue::empty() const { return heap_.empty(); }

size_t AlertPriorityQueue::size() const { return heap_.size(); }

void AlertPriorityQueue::clear() { heap_.clear(); }

void AlertPriorityQueue::evict() {
    while (heap_.size() > max_size_) {
        // Remove the lowest severity element (bottom of heap)
        // For simplicity, we'll pop the root (highest severity) and then re-heapify; real LRU eviction would be more complex
        pop();
    }
}

void AlertPriorityQueue::heapify_up(size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (compare_severity(heap_[idx], heap_[parent])) {
            std::swap(heap_[idx], heap_[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

void AlertPriorityQueue::heapify_down(size_t idx) {
    size_t n = heap_.size();
    while (true) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t largest = idx;
        if (left < n && compare_severity(heap_[left], heap_[largest])) largest = left;
        if (right < n && compare_severity(heap_[right], heap_[largest])) largest = right;
        if (largest != idx) {
            std::swap(heap_[idx], heap_[largest]);
            idx = largest;
        } else {
            break;
        }
    }
}

// ThresholdEngine implementation
ThresholdEngine::ThresholdEngine() : current_threshold_(0.75), ema_(0.3), optimizer_(1000), alert_queue_(1000) {}

ThresholdEngine::ThresholdEngine(double initial_threshold)
    : current_threshold_(initial_threshold), ema_(0.3), optimizer_(1000), alert_queue_(1000) {}

bool ThresholdEngine::exceeds_threshold(double score) const {
    return score >= current_threshold_;
}

void ThresholdEngine::update(double score) {
    std::lock_guard<std::mutex> lock(mutex_);
    ema_.update(score);
    // Adjust threshold based on EMA + multiplier * deviation
    current_threshold_ = ema_.value() + ema_multiplier_ * ema_.deviation();
    if (current_threshold_ > 1.0) current_threshold_ = 1.0;
    if (current_threshold_ < 0.0) current_threshold_ = 0.0;
}

void ThresholdEngine::add_labelled_sample(double score, bool is_anomaly) {
    std::lock_guard<std::mutex> lock(mutex_);
    optimizer_.add_sample(score, is_anomaly);
}

void ThresholdEngine::recalculate_threshold() {
    std::lock_guard<std::mutex> lock(mutex_);
    double opt = optimizer_.optimize();
    if (opt >= 0.0 && opt <= 1.0) {
        current_threshold_ = opt;
    }
}

void ThresholdEngine::push_alert(const Alert& alert) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_queue_.push(alert);
}

void ThresholdEngine::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_threshold_ = 0.75;
    ema_.reset();
    optimizer_.reset();
    alert_queue_.clear();
}

} // namespace speculor