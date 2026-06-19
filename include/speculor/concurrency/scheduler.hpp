#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

namespace speculor {

class TaskScheduler {
public:
    struct Task {
        std::string name;
        std::function<void()> func;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point next_run;
        bool repeat{true};
    };

    TaskScheduler();
    explicit TaskScheduler(size_t num_threads);
    ~TaskScheduler();

    void add_task(const Task& task);
    void remove_task(const std::string& name);
    void start();
    void stop();
    bool is_running() const;

private:
    std::vector<Task> tasks_;
    mutable std::mutex tasks_mutex_;
    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};
    size_t num_threads_{1};

    void scheduler_loop();
    void execute_task(const Task& task);
};

} // namespace speculor