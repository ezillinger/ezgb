#pragma once

#pragma warning(push, 1)
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <optional>

namespace fs = std::filesystem;

namespace ez {

    #define EZ_FAIL() assert(false)
    #define EZ_ASSERT(statement) assert(statement)

    #ifndef NDEBUG
        #define EZ_ENSURE(statement) assert(statement)
    #else
    #undef NDEBUG
        #define EZ_ENSURE(statement) assert(statement)
    #define NDEBUG 1
    #endif


    #define EZ_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
    #define EZ_INFO(...) SPDLOG_INFO(__VA_ARGS__)
    #define EZ_WARN(...) SPDLOG_WARN(__VA_ARGS__)
    #define EZ_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)


    inline uint8_t highByte(uint16_t val) { return static_cast<uint8_t>(val >> 8); }
    inline uint8_t lowByte(uint16_t val) { return static_cast<uint8_t>(val & 0x00FF); }
    inline void setHighByte(uint16_t& val, uint8_t to) { val = (( val & 0x00FF) | (static_cast<uint16_t>(to) << 8)); }
    inline void setLowByte(uint16_t& val, uint8_t to) { val = (( val & 0xFF00) | static_cast<uint16_t>(to)); }
}
