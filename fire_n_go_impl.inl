// fire_n_go_impl.inl
// This file contains the implementation of template functions declared in fire_n_go.hpp

#include <string>
#include <utility>

// This is an internal function declaration, not meant for external use.
// It retrieves the singleton instance of the thread pool.
namespace util {
    extern ThreadPool* get_thread_pool_instance();
}

template<typename Callable>
void util::fire_and_forget(std::string_view task_name, Callable&& task)
    requires std::invocable<Callable&&>
{
    ThreadPool* pool_instance = get_thread_pool_instance();
    if (!pool_instance) {
        log::print<log::Level::Error>("TaskRunner", "fire_and_forget called but thread pool is not available.");
        return;
    }

    std::string name_copy(task_name);

    // SONARCLOUD FIX: The lambda now explicitly captures the variables it needs.
    // 'name' is moved in, and 'work' is perfectly forwarded via an init-capture.
    auto wrapped_task = [name = std::move(name_copy), work = std::forward<Callable>(task)]() mutable {
        log::print<log::Level::Info>("TaskRunner", "Starting task: '{}'", name);
        try {
            std::invoke(std::move(work));
            log::print<log::Level::Info>("TaskRunner", "Finished task: '{}'", name);
        } catch (const std::exception& e) {
            const char* error_what = e.what();
            log::print<log::Level::Error>("TaskRunner", "Exception caught in task '{}': {}", name, error_what);
        } catch (...) {
            log::print<log::Level::Error>("TaskRunner", "Unknown exception caught in task '{}'", name);
        }
    };

    pool_instance->enqueue(std::move(wrapped_task));
}

template<typename F>
void util::ThreadPool::enqueue(F&& task) {
    {
        std::scoped_lock lock(m_queue_mutex);
        m_tasks.emplace(std::forward<F>(task));
    }
    m_condition.notify_one();
}
