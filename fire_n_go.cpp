// thread_pool.cpp
#include "fire_n_go.hpp"
#include <memory>

namespace util {

// Definition of the global thread pool instance.
std::unique_ptr<ThreadPool> global_thread_pool;

// --- Automatic Lifecycle Management ---

namespace { // Anonymous namespace for internal linkage

/**
 * @struct ThreadPoolManager
 * @brief Manages the lifecycle of the global thread pool via RAII.
 *
 * A static instance of this struct ensures its constructor runs before main()
 * and its destructor runs after main() returns, automating setup and teardown.
 */
struct ThreadPoolManager {
    ThreadPoolManager() {
        size_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2; // Fallback for systems where detection fails.
        global_thread_pool = std::make_unique<ThreadPool>(num_threads);
        log::print<log::Level::Info>("ThreadPool", "Automatic thread pool initialized with {} threads.", num_threads);
    }

    ~ThreadPoolManager() {
        if (global_thread_pool) {
            log::print<log::Level::Info>("ThreadPool", "Automatic thread pool shutting down...");
            // The unique_ptr's reset() calls the ThreadPool destructor.
            global_thread_pool.reset();
            log::print<log::Level::Info>("ThreadPool", "Automatic thread pool has been shut down.");
        }
    }
};

// This static instance guarantees the constructor/destructor are called.
static ThreadPoolManager manager_instance;

} // namespace

// --- ThreadPool Method Implementations ---

ThreadPool::ThreadPool(size_t num_threads) {
    start(num_threads);
}

ThreadPool::~ThreadPool() {
    // Request all threads to stop.
    m_stop_source.request_stop();
    // Wake up all threads so they can check their stop token.
    m_condition.notify_all();
    // The std::jthread destructors in m_workers will automatically join.
}

void ThreadPool::start(size_t num_threads) {
    m_workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        // Create a jthread, passing it a stop_token from our source.
        m_workers.emplace_back([this](std::stop_token stoken) {
            worker_loop(std::move(stoken));
        }, m_stop_source.get_token());
    }
}

void ThreadPool::worker_loop(std::stop_token stoken) {
    // Loop until a stop is requested.
    while (!stoken.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            // Wait until there's a task or a stop is requested.
            m_condition.wait(lock, [&] {
                return stoken.stop_requested() || !m_tasks.empty();
            });

            // If a stop was requested and the queue is empty, we can exit.
            if (stoken.stop_requested() && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        // Execute the task without holding the lock.
        task();
    }
}

} // namespace util
