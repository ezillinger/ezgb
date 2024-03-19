#include "base.h"

namespace ez {

const char* to_string(LogLevel level) {
    switch (level) {
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::CRITICAL:
        return "CRITICAL";
    }
    abort();
}

static auto as_local(const std::chrono::system_clock::time_point& tp) {
    return std::chrono::zoned_time{std::chrono::current_zone(), tp};
}

std::string to_string(const std::chrono::system_clock::time_point& tp) {
    return std::format("{:%T}", as_local(tp));
}

std::string to_string(const std::source_location& source) {
    return std::format(
        "{}:{} {}",
        std::filesystem::path(source.file_name()).filename().string(),
        source.line(), source.function_name());
}

} // namespace ez