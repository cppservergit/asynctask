// fire_n_go.cpp
#include "fire_n_go.hpp"
#include <memory>

namespace util {

namespace { // Anonymous namespace for internal linkage

    // Meyers' Singleton pattern to manage the global thread pool instance.
    // It's thread-safe for initialization and avoids non-const global variables.
    std::unique_ptr<ThreadPool>& get_pool_ptr() {
        static std::unique_ptr<ThreadPool> global_thread_pool_ptr;
        return global_thread_pool_ptr;
    }

    // RAII manager for the thread pool's lifecycle.
    struct ThreadPoolManager {
        ThreadPoolManager() {
            size_t num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 2; // Fallback
            get_pool_ptr() = std::make_unique<ThreadPool>(num_threads);
            log::print<log::Level::Info>("ThreadPool", "Automatic thread pool initialized with {} threads.", num_threads);
        }

        ~ThreadPoolManager() {
            if (get_pool_ptr()) {
                log::print<log::Level::Info>("ThreadPool", "Automatic thread pool shutting down...");
                get_pool_ptr().reset();
                log::print<log::Level::Info>("ThreadPool", "Automatic thread pool has been shut down.");
            }
        }

        // Prevent copying/moving of the manager.
        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&) = delete;
    };

    // This static instance guarantees the constructor/destructor are called at program start/end.
    ThreadPoolManager manager_instance;

} // namespace

// --- Public function to access the pool ---
// This is the single, controlled point of access for the header's template function.
ThreadPool* get_thread_pool_instance() {
    return get_pool_ptr().get();
}


// --- ThreadPool Method Implementations ---

ThreadPool::ThreadPool(size_t num_threads) {
    start(num_threads);
}

ThreadPool::~ThreadPool() {
    m_stop_source.request_stop();
    m_condition.notify_all();
}

void ThreadPool::start(size_t num_threads) {
    m_workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        m_workers.emplace_back([this](std::stop_token stoken) {
            worker_loop(std::move(stoken));
        }, m_stop_source.get_token());
    }
}

void ThreadPool::worker_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock lock(m_queue_mutex);
            m_condition.wait(lock, [this, &stoken] {
                return stoken.stop_requested() || !m_tasks.empty();
            });

            if (stoken.stop_requested() && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task();
    }
}

} // namespace util
