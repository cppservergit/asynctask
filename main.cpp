// main.cpp
#include "fire_n_go.hpp"
#include "logger.hpp"
#include <chrono>
#include <stdexcept> // For std::runtime_error

// A function to simulate work.
void long_running_database_query() {
    util::log::print<util::log::Level::Info>("Database", "Performing database query...");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// A function that intentionally throws an error to test logging.
void failing_task() {
    util::log::print<util::log::Level::Warning>("FailingTask", "This task is about to throw an exception.");
    throw std::runtime_error("Simulated runtime failure!");
}

int main() {
    // The ThreadPoolManager handles initialization and shutdown automatically.
    // There is no need for manual setup or teardown calls.

    util::log::print<util::log::Level::Info>("Application", "Main function started. Dispatching tasks...");

    // --- Standard Info Log ---
    util::fire_and_forget("Update User Cache", [] {
        util::log::print<util::log::Level::Info>("Cache", "Updating user cache...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    });

    // --- Debug Log Test Case ---
    // This message will only appear if compiled with debug logs enabled.
    util::fire_and_forget("Debug Info", []{
        util::log::print<util::log::Level::Debug>("Debug", "This is a detailed debug message for developers.");
    });

    // --- Error Log and Stack Trace Test Case ---
    // This task will fail and trigger an Error log.
    util::fire_and_forget("Simulate Failure", failing_task);


    util::log::print<util::log::Level::Info>("Application", "Main thread is continuing with other work...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    util::log::print<util::log::Level::Info>("Application", "Main function is about to exit. Pool shutdown will be automatic.");
    return 0;
}
