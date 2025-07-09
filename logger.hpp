// logger.hpp
#pragma once

#include <iostream>
#include <string_view>
#include <chrono>
#include <format>       // C++20 formatting library
#include <syncstream>   // C++20 for thread-safe streaming
#include <thread>
#include <source_location> // C++20 for capturing source info
#include <iterator>     // For std::ostream_iterator
#include <concepts>     // For std::convertible_to
#include <sstream>      // For std::stringstream to handle thread::id

// Conditionally include <stacktrace> only if enabled via the Makefile
#ifdef USE_STACKTRACE
#include <stacktrace>   // C++23 for automatic stack traces on error
#endif

namespace util::log {

// --- Compile-time configuration for debug logging ---
// This allows the build system to control whether debug logs are enabled.
// Example: g++ -DENABLE_DEBUG_LOGS=1 ...
#ifndef ENABLE_DEBUG_LOGS
#define ENABLE_DEBUG_LOGS 0 // Disabled by default for release builds
#endif
// This constexpr flag will be used to compile out debug logs entirely.
constexpr bool debug_logging_enabled = (ENABLE_DEBUG_LOGS == 1);


// Defines the severity level of a log message.
enum class Level {
    Debug,
    Info,
    Warning,
    Error
};

// Converts a log level enum to its string representation.
constexpr std::string_view level_to_string(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
    }
    return "UNKNOWN";
}

// --- Primary print function ---
// This single function uses 'if constexpr' to handle both formatted and simple messages.
// This avoids the function overload ambiguity issues.
template<Level level, typename Fmt, typename... Args>
void print(std::string_view area, Fmt&& fmt, Args&&... args) {
    // If this is a debug message and debug logging is disabled, compile out the call.
    if constexpr (level == Level::Debug && !debug_logging_enabled) {
        return;
    }

    // osyncstream ensures the entire message is written to the underlying stream atomically.
    std::osyncstream synced_out(level == Level::Error ? std::cerr : std::cout);
    
    // Use a preprocessor check to handle compiler-specific support for formatting std::thread::id.
    // GCC 13 and older require a stringstream workaround.
    // GCC 14+ and other modern compilers can format it directly.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 14
    // Workaround for GCC < 14
    std::stringstream thread_id_ss;
    thread_id_ss << std::this_thread::get_id();
    synced_out << std::format("[{:<7}] [{:^12}] [Thread:{}] ", level_to_string(level), area, thread_id_ss.str());
#else
    // Standard-compliant way for modern compilers (GCC 14+, Clang, MSVC)
    synced_out << std::format("[{:<7}] [{:^12}] [Thread:{}] ", level_to_string(level), area, std::this_thread::get_id());
#endif


    // Use 'if constexpr' to differentiate between formatted and simple calls at compile time.
    if constexpr (sizeof...(args) > 0) {
        // Formatted call: Use vformat_to for efficiency.
        std::vformat_to(
            std::ostream_iterator<char>(synced_out),
            std::forward<Fmt>(fmt),
            std::make_format_args(std::forward<Args>(args)...)
        );
    } else {
        // Simple message call: Just stream the message.
        synced_out << std::forward<Fmt>(fmt);
    }
    
    synced_out << '\n';

    // Conditionally compile stack trace logic only if enabled.
    #ifdef USE_STACKTRACE
    if constexpr (level == Level::Error && debug_logging_enabled) {
        synced_out << "--- Stack Trace ---\n" << std::stacktrace::current() << "-------------------\n";
    }
    #endif
}

} // namespace util::log
