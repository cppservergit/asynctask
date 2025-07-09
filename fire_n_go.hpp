// fire_n_go.hpp
#pragma once

#include "logger.hpp" // For logging
#include <concepts>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token> // For std::stop_source and std::stop_token
#include <string>     // For std::string
#include <string_view>
#include <thread> // For std::jthread
#include <utility>
#include <vector>

namespace util {

// Forward declaration for the ThreadPool class
class ThreadPool;

// Declaration of the global pointer. Its definition is in the .cpp file.
// It must be declared here so the header-only fire_and_forget function can access it.
extern std::unique_ptr<ThreadPool> global_thread_pool;

/**
 * @brief Dispatches a task to the global thread pool for immediate, asynchronous execution.
 *
 * This function's implementation is in the header because it is a function template.
 *
 * @tparam Callable A concept ensuring the passed type is a callable that can be invoked without arguments.
 * @param task_name A descriptive name for the task, used for logging.
 * @param task The callable object (lambda, function pointer, etc.) to be executed.
 */
template<std::invocable<> Callable>
void fire_and_forget(std::string_view task_name, Callable&& task);


/**
 * @class ThreadPool
 * @brief Manages a pool of worker jthreads to execute tasks concurrently.
 *
 * @note This class is an internal implementation detail.
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Delete copy and move operations to enforce singleton-like behavior.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<typename F>
    void enqueue(F&& task) {
        {
            std::scoped_lock lock(m_queue_mutex);
            m_tasks.emplace(std::forward<F>(task));
        }
        m_condition.notify_one();
    }

private:
    void start(size_t num_threads);
    void worker_loop(std::stop_token stoken);

    std::vector<std::jthread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::stop_source m_stop_source;
};

// --- Template Implementation ---

template<std::invocable<> Callable>
void fire_and_forget(std::string_view task_name, Callable&& task) {
    if (!global_thread_pool) {
        log::print<log::Level::Error>("TaskRunner", "fire_and_forget called but thread pool is not available.");
        return;
    }

    std::string name_copy(task_name);

    auto wrapped_task = [name = std::move(name_copy), work = std::forward<Callable>(task)]() mutable {
        log::print<log::Level::Info>("TaskRunner", "Starting task: '{}'", name);
        try {
            std::invoke(std::move(work));
            log::print<log::Level::Info>("TaskRunner", "Finished task: '{}'", name);
        } catch (const std::exception& e) {
            // FIX: Store the result of e.what() in a local variable to ensure it's an
            // lvalue, which resolves the template argument binding error.
            const char* error_what = e.what();
            log::print<log::Level::Error>("TaskRunner", "Exception caught in task '{}': {}", name, error_what);
        } catch (...) {
            log::print<log::Level::Error>("TaskRunner", "Unknown exception caught in task '{}'", name);
        }
    };

    global_thread_pool->enqueue(std::move(wrapped_task));
}

} // namespace util
