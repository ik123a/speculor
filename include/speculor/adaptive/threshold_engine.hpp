#pragma once
#include "speculor/core/alert.hpp"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <functional>
#include <atomic>

namespace speculor {

class Event;

/// Computes Exponential Moving Average of anomaly scores
class EMACalculator {
public:
    explicit EMACalculator(double alpha = 0.3);

    void update(double value);
    double value() const { return ema_value_; }
    double deviation() const { return ema_deviation_; }
    void reset();

private:
    double alpha_{0.3};
    double ema_value_{0.0};
    double ema_deviation_{0.0};
    bool initialized_{false};
};

/// Finds optimal threshold using dynamic programming on a sliding window
class ThresholdOptimizer {
public:
    explicit ThresholdOptimizer(size_t window_size = 1000);

    void add_sample(double score, bool is_anomaly);
    double optimize();
    void reset();
    size_t sample_count() const;

private:
    struct Sample {
        double score;
        bool is_anomaly;
    };

    std::vector<Sample> window_;
    size_t window_size_{1000};
    size_t insert_idx_{0};

    double compute_f1(double threshold) const;
};

/// Max-heap priority queue for alerts with LRU eviction
class AlertPriorityQueue {
public:
    explicit AlertPriorityQueue(size_t max_size = 1000);

    void push(const Alert& alert);
    Alert top() const;
    void pop();
    bool empty() const;
    size_t size() const;
    void clear();
    void evict();

private:
    std::vector<Alert> heap_;
    size_t max_size_{1000};

    void heapify_up(size_t idx);
    void heapify_down(size_t idx);
    static bool compare_severity(const Alert& a, const Alert& b);
};

/// Main adaptive threshold engine
class ThresholdEngine {
public:
    ThresholdEngine();
    explicit ThresholdEngine(double initial_threshold);
    ~ThresholdEngine() = default;

    bool exceeds_threshold(double score) const;
    void update(double score);
    void add_labelled_sample(double score, bool is_anomaly);
    void recalculate_threshold();
    void push_alert(const Alert& alert);
    void reset();

    double current_threshold() const { return current_threshold_; }
    double ema_value() const { return ema_.value(); }
    double ema_deviation() const { return ema_.deviation(); }
    const AlertPriorityQueue& alert_queue() const { return alert_queue_; }

private:
    double current_threshold_{0.75};
    double ema_alpha_{0.3};
    double ema_multiplier_{2.0};

    EMACalculator ema_;
    ThresholdOptimizer optimizer_;
    AlertPriorityQueue alert_queue_;
    mutable std::mutex mutex_;
};

} // namespace speculor
