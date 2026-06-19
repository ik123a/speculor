#include "speculor/core/engine.hpp"
#include "speculor/core/config.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/alert.hpp"
#include "speculor/concurrency/thread_pool.hpp"
#include "speculor/concurrency/ring_buffer.hpp"
#include "speculor/concurrency/scheduler.hpp"
#include "speculor/storage/persistent_store.hpp"
#include "speculor/detectors/bloom_detector.hpp"
#include "speculor/detectors/cms_detector.hpp"
#include "speculor/detectors/kdtree_detector.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace speculor {

SpeculorEngine::SpeculorEngine()
    : input_queue_(std::make_unique<RingBuffer<Event>>(1024)),
      pool_(std::make_unique<ThreadPool>(4)),
      running_(false) {}

SpeculorEngine::~SpeculorEngine() {
    stop();
    if (running_) {
        // Ensure all threads are joined
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SpeculorEngine::configure(const Config& config) {
    config_ = config;
    // Configure pool
    if (pool_) {
        pool_->stop();  // Ensure we don't have stray threads
        pool_ = std::make_unique<ThreadPool>(config.thread_pool_size);
    }
    // Configure ring buffer
    if (input_queue_) {
        input_queue_ = std::make_unique<RingBuffer<Event>>(config.input_queue_size);
    }
    // Configure storage
    store_ = nullptr;
    if (config.enable_persistence && config.storage_path != "") {
        store_ = std::make_unique<RocksDbStore>();
        if (!store_->open(config.storage_path)) {
            throw std::runtime_error("Failed to open RocksDB store at " + config.storage_path);
        }
    }
    
    // Configure detectors pipeline
    pipeline_.clear_detectors();
    for (const auto& det_cfg : config.detectors) {
        if (det_cfg.type == "bloom") {
            size_t bits = 65536;
            size_t hashes = 4;
            double decay = 0.95;
            if (det_cfg.parameters.count("num_bits")) bits = std::stoull(det_cfg.parameters.at("num_bits"));
            if (det_cfg.parameters.count("num_hashes")) hashes = std::stoull(det_cfg.parameters.at("num_hashes"));
            if (det_cfg.parameters.count("decay_factor")) decay = std::stod(det_cfg.parameters.at("decay_factor"));
            pipeline_.add_detector(std::make_shared<BloomDetector>(bits, hashes, decay), 1.0, "bloom");
        } else if (det_cfg.type == "cms") {
            size_t width = 1024;
            size_t depth = 4;
            double decay = 0.95;
            if (det_cfg.parameters.count("width")) width = std::stoull(det_cfg.parameters.at("width"));
            if (det_cfg.parameters.count("depth")) depth = std::stoull(det_cfg.parameters.at("depth"));
            if (det_cfg.parameters.count("decay_factor")) decay = std::stod(det_cfg.parameters.at("decay_factor"));
            pipeline_.add_detector(std::make_shared<CMSDetector>(width, depth, decay), 1.0, "cms");
        } else if (det_cfg.type == "kdtree") {
            size_t max_pts = 10000;
            size_t k = 3;
            if (det_cfg.parameters.count("max_points")) max_pts = std::stoull(det_cfg.parameters.at("max_points"));
            if (det_cfg.parameters.count("k_neighbors")) k = std::stoull(det_cfg.parameters.at("k_neighbors"));
            pipeline_.add_detector(std::make_shared<KDTreeDetector>(max_pts, k), 1.0, "kdtree");
        }
    }

    // Configure snapshot timer dynamically if enabled
    if (config.enable_persistence && config.snapshot_interval_ms > 0) {
        scheduler_.add_task(TaskScheduler::Task{
            "snapshot_task",
            [this, config] {
                snapshot(config.storage_path + "/snapshot");
            },
            std::chrono::milliseconds(config.snapshot_interval_ms),
            std::chrono::steady_clock::now() + std::chrono::milliseconds(config.snapshot_interval_ms),
            true // repeat
        });
    }
}

void SpeculorEngine::configure(const std::string& config_path) {
    if (config_path.rfind(".yaml") != std::string::npos || config_path.rfind(".yml") != std::string::npos) {
        configure(Config::from_yaml(config_path));
    } else {
        configure(Config::from_json(config_path));
    }
}

std::optional<Alert> SpeculorEngine::process(const Event& event) {
    // Process synchronously in the current thread
    update_detectors(event);
    auto alert = generate_alert(event);
    if (alert) {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        recent_alerts_.push_back(*alert);
    }
    return alert;
}

void SpeculorEngine::process_async(const Event& event) {
    // Add to ring buffer
    if (!input_queue_->try_push(event)) {
        // Ring buffer full - optionally notify or drop
        std::cerr << "Ring buffer full, dropping event" << std::endl;
        return;
    }

    // Notify scheduler to process in background thread
    scheduler_.add_task(TaskScheduler::Task{
        "process_async_task",
        [this, event] {
            update_detectors(event);
            auto alert = generate_alert(event);
            if (alert) {
                std::lock_guard<std::mutex> lock(alerts_mutex_);
                recent_alerts_.push_back(*alert);
            }
        },
        std::chrono::milliseconds(0),
        std::chrono::steady_clock::now(),
        false // repeat
    });
}

void SpeculorEngine::update_detectors(const Event& event) {
    pipeline_.update(event);
}

std::optional<Alert> SpeculorEngine::generate_alert(const Event& event) {
    double score = pipeline_.score(event);
    threshold_.update(score);

    if (threshold_.exceeds_threshold(score)) {
        Severity sev = Severity::Medium;
        if (score > 0.9) sev = Severity::Critical;
        else if (score > 0.8) sev = Severity::High;
        else if (score > 0.6) sev = Severity::Low;

        return std::make_optional<Alert>(Alert{
            .severity = static_cast<uint32_t>(sev),
            .detector_source = "composite_pipeline",
            .timestamp = std::chrono::system_clock::now(),
            .score = score,
            .metric_name = event.metric_name,
            .metric_value = event.value,
            .description = "Anomaly detected: score " + std::to_string(score) + " exceeds threshold " + std::to_string(threshold_.current_threshold())
        });
    }

    return std::nullopt;
}

std::vector<Alert> SpeculorEngine::get_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    return recent_alerts_;
}

void SpeculorEngine::snapshot(const std::string& path) {
    std::cout << "Snapshot to " << path << std::endl;
    if (store_) {
        std::vector<uint8_t> pipeline_data = pipeline_.serialize();
        store_->put("pipeline", pipeline_data);
    }
}

void SpeculorEngine::restore(const std::string& path) {
    std::cout << "Restore from " << path << std::endl;
    if (store_) {
        std::vector<uint8_t> pipeline_data = store_->get("pipeline");
        if (!pipeline_data.empty()) {
            pipeline_.deserialize(pipeline_data);
        }
    }
}

void SpeculorEngine::stop() {
    running_ = false;
    if (pool_) {
        pool_->stop();
    }
    // Stop the scheduler
    scheduler_.stop();
}

} // namespace speculor