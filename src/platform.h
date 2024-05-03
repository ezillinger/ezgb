#pragma once

#if defined(__clang__)
    #define EZ_CLANG 1
    #pragma clang diagnostic ignored "-Wformat-security"

#elif defined(__GNUC__) || defined(__GNUG__)
    #define EZ_GCC 1
#elif defined(_MSC_VER)
    #define EZ_MSVC 1
#include <debugapi.h>
#endif

#if EZ_MSVC
    #define EZ_MSVC_WARN_PUSH(PP_NUMBER) \
        _Pragma(warning(push, PP_NUMBER));
    #define EZ_MSVC_WARN_POP \
        _Pragma(warning(pop));
    #define EZ_DEBUG_BREAK() DebugBreak()
#else
    #define EZ_MSVC_WARN_PUSH(PP_NUMBER)
    #define EZ_MSVC_WARN_POP()
    #define EZ_DEBUG_BREAK() raise(SIGTRAP)
#endif
