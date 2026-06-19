#include "speculor/concurrency/scheduler.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace speculor {

TaskScheduler::TaskScheduler() : running_(false), num_threads_(1) {}

TaskScheduler::TaskScheduler(size_t num_threads) : running_(false), num_threads_(num_threads) {}

TaskScheduler::~TaskScheduler() {
    stop();
}

void TaskScheduler::scheduler_loop() {
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(tasks_mutex_);
            auto now = std::chrono::steady_clock::now();
            for (auto& task : tasks_) {
                if (task.next_run <= now) {
                    execute_task(task);
                    if (task.repeat) {
                        task.next_run = now + task.interval;
                    }
                }
            }
            // Remove non-repeating tasks that have executed
            tasks_.erase(
                std::remove_if(tasks_.begin(), tasks_.end(),
                    [](const Task& t) { return !t.repeat && t.next_run <= std::chrono::steady_clock::now(); }),
                tasks_.end()
            );
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TaskScheduler::execute_task(const Task& task) {
    try {
        task.func();
    } catch (const std::exception& e) {
        std::cerr << "Scheduler task '" << task.name << "' exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Scheduler task '" << task.name << "' threw unknown exception" << std::endl;
    }
}

void TaskScheduler::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    for (size_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { scheduler_loop(); });
    }
}

void TaskScheduler::add_task(const Task& task) {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    tasks_.push_back(task);
}

void TaskScheduler::remove_task(const std::string& name) {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    tasks_.erase(
        std::remove_if(tasks_.begin(), tasks_.end(),
            [&](const Task& t) { return t.name == name; }),
        tasks_.end()
    );
}

void TaskScheduler::stop() {
    running_ = false;
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}

bool TaskScheduler::is_running() const {
    return running_.load();
}

} // namespace speculor