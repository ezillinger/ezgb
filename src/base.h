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


}
