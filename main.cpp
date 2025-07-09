// main.cpp
#include "fire_n_go.hpp"
#include "logger.hpp"
#include <chrono>
#include <stdexcept>

// The TaskFailure struct is now defined in fire_n_go.hpp

// A function to simulate work.
void long_running_database_query() {
    using enum util::log::Level;
    util::log::print<Info>("Database", "Performing database query...");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// A function that intentionally throws our dedicated exception type.
// SONARCLOUD FIX: Add [[noreturn]] attribute to signal that this function never returns.
[[noreturn]] void failing_task() {
    using enum util::log::Level;
    util::log::print<Warning>("FailingTask", "This task is about to throw an exception.");
    throw util::TaskFailure("Simulated runtime failure!");
}

int main() {
    using enum util::log::Level;

    // The ThreadPoolManager handles initialization and shutdown automatically.
    util::log::print<Info>("Application", "Main function started. Dispatching tasks...");

    // --- Standard Info Log ---
    util::fire_and_forget("Update User Cache", [] {
        util::log::print<Info>("Cache", "Updating user cache...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    });

    // --- Debug Log Test Case ---
    util::fire_and_forget("Debug Info", []{
        util::log::print<Debug>("Debug", "This is a detailed debug message for developers.");
    });

    // --- Error Log and Stack Trace Test Case ---
    util::fire_and_forget("Simulate Failure", failing_task);


    util::log::print<Info>("Application", "Main thread is continuing with other work...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    util::log::print<Info>("Application", "Main function is about to exit. Pool shutdown will be automatic.");
    return 0;
}
