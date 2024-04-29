#pragma once

#include "Platform.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <signal.h>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace ez {

inline constexpr void ez_assert(bool cond) { assert(cond); }

#ifndef NDEBUG
    #define EZ_ENSURE(statement) assert(statement)
#else
    #undef NDEBUG
    #define EZ_ENSURE(statement) assert(statement)
    #define NDEBUG 1
#endif

template <typename TTo, typename TFrom> inline TTo checked_cast(const TFrom&& from) {
    const auto result = static_cast<TTo>(from);
    ez_assert(static_cast<TFrom>(result) == from);
    return result;
}

namespace fs = std::filesystem;
namespace chrono = std::chrono;
using namespace std::literals::chrono_literals;
using namespace std::literals;

enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

const char* to_string(LogLevel level);
std::string to_string(const std::chrono::system_clock::time_point& tp);
std::string to_string(const std::source_location& source);

template <LogLevel level, typename... TArgs> struct log {
    log(std::string_view format, const TArgs&... args,
        std::source_location location = std::source_location::current()) {
        std::cout << std::format(
            "{} {} {} | {}\n", to_string(level), to_string(std::chrono::system_clock::now()),
            to_string(location),
            std::vformat(format, std::make_format_args(std::forward<const TArgs>(args)...)));

        if constexpr (level == LogLevel::CRITICAL || level == LogLevel::ERROR) {
            std::cout.flush();
        }
    }
};

#ifdef EZ_CLANG

template <typename... TArgs> struct log_info : log<LogLevel::INFO, TArgs...> {
    using log<LogLevel::INFO, TArgs...>::log;
};
template <typename... TArgs> log_info(std::string_view, TArgs...) -> log_info<TArgs...>;

template <typename... TArgs> struct log_warn : log<LogLevel::WARN, TArgs...> {
    using log<LogLevel::WARN, TArgs...>::log;
};
template <typename... TArgs> log_warn(std::string_view, TArgs...) -> log_warn<TArgs...>;

template <typename... TArgs> struct log_error : log<LogLevel::ERROR, TArgs...> {
    using log<LogLevel::ERROR, TArgs...>::log;
};
template <typename... TArgs> log_error(std::string_view, TArgs...) -> log_error<TArgs...>;

template <typename... TArgs> struct log_critical : log<LogLevel::CRITICAL, TArgs...> {
    using log<LogLevel::CRITICAL, TArgs...>::log;
};
template <typename... TArgs> log_critical(std::string_view, TArgs...) -> log_critical<TArgs...>;

#else
template <LogLevel level, typename... TArgs>
log(std::string_view, TArgs&&...) -> log<level, TArgs...>;
template <typename... TArgs> using log_info = log<LogLevel::INFO, TArgs...>;
template <typename... TArgs> using log_warn = log<LogLevel::WARN, TArgs...>;
template <typename... TArgs> using log_error = log<LogLevel::ERROR, TArgs...>;
template <typename... TArgs> using log_critical = log<LogLevel::CRITICAL, TArgs...>;
#endif

template <typename... TArgs> [[noreturn]] void fail(TArgs... args) {
    log_critical(std::forward<const TArgs&>(args)...);
    abort();
}

// sets from == to, returns true if changes
template <typename T> inline bool update(T& from, const T& to) {
    const auto changed = from != to;
    from = to;
    return changed;
}

inline auto operator"" _format(const char* s, size_t n) {
    return [=](auto&&... args) {
        return std::vformat(std::string_view(s, n), std::make_format_args(args...));
    };
}

template <typename T>
concept is_enum = std::is_enum_v<T>;

template <typename T> struct Range {
    constexpr Range() : Range(T{}, T{}) {}
    constexpr Range(T a, T b) : m_min(std::min(a, b)), m_max(std::max(a, b)) {}
    inline constexpr T width() const { return m_max - m_min; }
    constexpr void extend(const T&& v) {
        m_min = std::min(m_min, v);
        m_max = std::max(m_max, v);
    }
    constexpr void extend(const Range<T>& other) {
        m_min = std::min(m_min, other.m_min);
        m_max = std::max(m_max, other.m_max);
    }

    inline constexpr bool containsExclusive(const T& v) const { return v >= m_min && v < m_max; }
    inline constexpr bool containsInclusive(const T& v) const { return v >= m_min && v <= m_max; }

    T m_min;
    T m_max;
};

using iRange = Range<int>;
using fRange = Range<float>;
using dRange = Range<double>;

// unary + operator converts enum to underlying type
template <typename T>
    requires is_enum<T>
constexpr std::underlying_type<T>::type operator+(T e) {
    return static_cast<std::underlying_type<T>::type>(e);
}

class Stopwatch {
  public:
    template <typename TDuration = chrono::milliseconds> TDuration elapsed() {
        return chrono::duration_cast<TDuration>(chrono::steady_clock::now() - m_start);
    }

    void reset() { m_start = chrono::steady_clock::now(); }

    template <typename TDuration> bool lapped(TDuration t) {
        if (elapsed<TDuration>() > t) {
            reset();
            return true;
        }
        return false;
    }

  private:
    chrono::steady_clock::time_point m_start = chrono::steady_clock::now();
};

// easy way to delete/default copy/move constructors
#define EZ_DEFINE_COPY_MOVE(PP_CLASS, PP_COPY, PP_MOVE) \
    PP_CLASS(const PP_CLASS&) = PP_COPY; \
    void operator=(const PP_CLASS&) = PP_COPY; \
    PP_CLASS(PP_CLASS&&) = PP_MOVE; \
    void operator=(PP_CLASS&&) = PP_MOVE; 

struct rgba8 {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

} // namespace ez
