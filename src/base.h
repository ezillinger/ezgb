#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace filesystem = std::filesystem;

namespace ez {

    #define EZ_ASSERT(statement) assert(statement)

    #ifndef NDEBUG
        #define EZ_ENSURE(statement) assert(statement)
    #else
    #undef NDEBUG
        #define EZ_ENSURE(statement) assert(statement)
    #define NDEBUG 1
    #endif

    #define EZ_INFO(...) SPDLOG_INFO(__VA_ARGS__)
    #define EZ_WARN(...) SPDLOG_WARN(__VA_ARGS__)
    #define EZ_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)

}
