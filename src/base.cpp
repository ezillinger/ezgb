#include "Base.h"

namespace ez {

const char* to_string(LogLevel level) {

    switch (level) {
        case LogLevel::INFO:     return "\e[0;32m[INFO]\e[0m";
        case LogLevel::WARN:     return "\e[0;33m[WARN]\e[0m";
        case LogLevel::ERROR:    return "\e[0;31m[ERROR]\e[0m";
        case LogLevel::CRITICAL: return "\e[0;31m[CRITICAL]\e[0m";
    }
    abort();

}

static auto as_local(const std::chrono::system_clock::time_point& tp) {
    #if EZ_CLANG
    // todo, implement this
    return tp;
    #else
    return std::chrono::zoned_time{std::chrono::current_zone(), tp};
    #endif
}

std::string to_string(const std::chrono::system_clock::time_point& tp) {
    return std::format("{:%T}", as_local(tp));
}

std::string to_string(const std::source_location& source) {
    return std::format("{}:{} {}", std::filesystem::path(source.file_name()).filename().string(),
                       source.line(), source.function_name());
}

} // namespace ez