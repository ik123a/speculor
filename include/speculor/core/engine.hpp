#pragma once
#include "speculor/core/event.hpp"
#include "speculor/core/alert.hpp"
#include "speculor/core/config.hpp"
#include "speculor/detectors/composite_detector.hpp"
#include "speculor/adaptive/threshold_engine.hpp"
#include "speculor/concurrency/ring_buffer.hpp"
#include "speculor/concurrency/thread_pool.hpp"
#include "speculor/concurrency/scheduler.hpp"
#include "speculor/storage/persistent_store.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <mutex>
#include <atomic>

namespace speculor {

class SpeculorEngine {
public:
    SpeculorEngine();
    ~SpeculorEngine();

    void configure(const Config& config);
    void configure(const std::string& config_path);

    /// Synchronous detection: process one event and return alert if anomalous
    std::optional<Alert> process(const Event& event);

    /// Asynchronous detection: enqueue event for background processing
    void process_async(const Event& event);

    /// Retrieve recent alerts
    std::vector<Alert> get_alerts() const;

    /// Serialize current state to disk
    void snapshot(const std::string& path);

    /// Load state from disk
    void restore(const std::string& path);

    /// Stop the engine and all background threads
    void stop();

    bool is_running() const { return running_.load(); }

private:
    CompositeDetector pipeline_;
    ThresholdEngine threshold_;
    std::unique_ptr<ThreadPool> pool_;
    std::unique_ptr<RingBuffer<Event>> input_queue_;
    std::unique_ptr<PersistentStore> store_;
    TaskScheduler scheduler_;
    Config config_;

    std::atomic<bool> running_{false};
    mutable std::mutex alerts_mutex_;
    std::vector<Alert> recent_alerts_;

    void update_detectors(const Event& event);
    std::optional<Alert> generate_alert(const Event& event);
    void async_worker();
};

} // namespace speculor
