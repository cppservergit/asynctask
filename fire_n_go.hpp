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

// SONARCLOUD FIX: The extern global variable has been removed to avoid non-const globals.
// Access to the pool is now handled internally within the .cpp file.

/**
 * @brief Dispatches a task to the global thread pool for immediate, asynchronous execution.
 *
 * This function's implementation is in the header because it is a function template.
 *
 * @tparam Callable The deduced type of the callable object.
 * @param task_name A descriptive name for the task, used for logging.
 * @param task The callable object (lambda, function pointer, etc.) to be executed.
 */
template<typename Callable>
void fire_and_forget(std::string_view task_name, Callable&& task)
    // This requires clause is a more precise way to constrain a forwarding reference.
    // It ensures that the type, after being perfectly forwarded, is invocable.
    requires std::invocable<Callable&&>;


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
    void enqueue(F&& task);

private:
    void start(size_t num_threads);
    void worker_loop(std::stop_token stoken);

    std::vector<std::jthread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    std::stop_source m_stop_source;
};

// The implementation of template functions must be in the header.
#include "fire_n_go_impl.inl"

} // namespace util
