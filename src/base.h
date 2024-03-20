#pragma once

#include "platform.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <format>
#include <cassert>
#include <chrono>


namespace ez {

#define EZ_FAIL(PP_MESSAGE) assert(false)
#define EZ_ASSERT(statement) assert(statement)

#ifndef NDEBUG
    #define EZ_ENSURE(statement) assert(statement)
#else
#undef NDEBUG
    #define EZ_ENSURE(statement) assert(statement)
#define NDEBUG 1
#endif

    template<typename TTo, typename TFrom>
    inline TTo checked_cast(const TFrom&& from ) {
        const auto result = static_cast<TTo>(from);
        EZ_ASSERT(static_cast<TFrom>(result) == from);
        return result;
    }

    namespace fs = std::filesystem;
    namespace chrono = std::chrono;

    enum class LogLevel { INFO, WARN, ERROR, CRITICAL };

    const char* to_string(LogLevel level);
    std::string to_string(const std::chrono::system_clock::time_point& tp);
    std::string to_string(const std::source_location& source);

    template <LogLevel level, typename... TArgs> struct log {
        log(std::string_view format, TArgs&&... args, std::source_location location = std::source_location::current()) {
            std::cout << std::format("[{}] {} {} | {}\n", to_string(level), to_string(std::chrono::system_clock::now()), to_string(location),
                                    std::vformat(format, std::make_format_args(std::forward<TArgs>(args)...)));

            if constexpr (level == LogLevel::CRITICAL) {
                std::cout.flush();
                abort();
            }
        }
    };

    template <LogLevel level, typename... TArgs> log(std::string_view, TArgs&&...) -> log<level, TArgs...>;
    template <typename... TArgs> using log_info = log<LogLevel::INFO, TArgs...>;
    template <typename... TArgs> using log_warn = log<LogLevel::WARN, TArgs...>;
    template <typename... TArgs> using log_error = log<LogLevel::ERROR, TArgs...>;
    template <typename... TArgs> using fail = log<LogLevel::CRITICAL, TArgs...>;

    inline auto operator"" _format(const char *s, size_t n) {
        return [=](auto &&...args) {
            return std::vformat(std::string_view(s, n), std::make_format_args(args...));
        };
    }

    template <typename T>
    concept is_enum = std::is_enum_v<T>;

    
    // unary + operator converts enum to underlying type
    template <typename T> requires is_enum<T>
    std::underlying_type<T>::type operator+(T e) { return static_cast<std::underlying_type<T>::type>(e); }

    inline constexpr uint8_t highByte(uint16_t val) { return static_cast<uint8_t>(val >> 8); }
    inline constexpr uint8_t lowByte(uint16_t val) { return static_cast<uint8_t>(val & 0x00FF); }
    inline constexpr void setHighByte(uint16_t& val, uint8_t to) { val = (( val & 0x00FF) | (static_cast<uint16_t>(to) << 8)); }
    inline constexpr void setLowByte(uint16_t& val, uint8_t to) { val = (( val & 0xFF00) | static_cast<uint16_t>(to)); }

}
