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
#ifndef ENABLE_DEBUG_LOGS
#define ENABLE_DEBUG_LOGS 0 // Disabled by default for release builds
#endif
constexpr bool debug_logging_enabled = (ENABLE_DEBUG_LOGS == 1);


// Defines the severity level of a log message.
enum class Level {
    Debug,
    Info,
    Warning,
    Error
};

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
template<Level level, typename Fmt, typename... Args>
void print(std::string_view area, Fmt&& fmt, Args&&... args) {
    if constexpr (level == Level::Debug && !debug_logging_enabled) {
        return;
    }

    std::osyncstream synced_out(level == Level::Error ? std::cerr : std::cout);
    
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 14
    std::stringstream thread_id_ss;
    thread_id_ss << std::this_thread::get_id();
    synced_out << std::format("[{:<7}] [{:^12}] [Thread:{}] ", level_to_string(level), area, thread_id_ss.str());
#else
    synced_out << std::format("[{:<7}] [{:^12}] [Thread:{}] ", level_to_string(level), area, std::this_thread::get_id());
#endif

    if constexpr (sizeof...(args) > 0) {
        // SONARCLOUD FIX: Use Class Template Argument Deduction (CTAD) for std::ostream_iterator.
        // Instead of std::ostream_iterator<char>(synced_out), we can just use std::ostream_iterator(...).
        std::vformat_to(
            std::ostream_iterator(synced_out),
            std::forward<Fmt>(fmt),
            std::make_format_args(std::forward<Args>(args)...)
        );
    } else {
        synced_out << std::forward<Fmt>(fmt);
    }
    
    synced_out << '\n';

    #ifdef USE_STACKTRACE
    if constexpr (level == Level::Error && debug_logging_enabled) {
        synced_out << "--- Stack Trace ---\n" << std::stacktrace::current() << "-------------------\n";
    }
    #endif
}

} // namespace util::log
