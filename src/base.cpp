#include "Base.h"

namespace ez {

const char* to_string(LogLevel level) {

#define EZ_RED "\e[0;31m"
#define EZ_GREEN "\e[0;32m"
#define EZ_YELLOW "\e[0;33m"
#define EZ_RESET "\e[0m"

    switch (level) {
        case LogLevel::INFO:     return EZ_GREEN "[INFO]" EZ_RESET;
        case LogLevel::WARN:     return EZ_YELLOW "[WARN]" EZ_RESET;
        case LogLevel::ERROR:    return EZ_RED "[ERROR]" EZ_RESET;
        case LogLevel::CRITICAL: return EZ_RED "[CRITICAL]" EZ_RESET;
    }
    abort();

#undef EZ_RED
#undef EZ_GREEN
#undef EZ_YELLOW
#undef EZ_RESET 

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