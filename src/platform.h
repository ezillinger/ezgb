#pragma once

#if defined(__clang__)
    #define EZ_CLANG 1
    #pragma clang diagnostic ignored "-Wformat-security"

#elif defined(__GNUC__) || defined(__GNUG__)
    #define EZ_GCC 1
#elif defined(_MSC_VER)
    #define EZ_MSVC 1
    // disable polluting #defines
    #define NOGDI
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN 1
    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
    #include <debugapi.h>
#endif

#if EZ_MSVC
    #define EZ_MSVC_WARN_PUSH() __pragma(warning(push))
    #define EZ_MSVC_WARN_POP() __pragma(warning(pop))
    #define EZ_MSVC_WARN_DISABLE(PP_ERROR_NO) __pragma(warning(disable : PP_ERROR_NO))
    #define EZ_DEBUG_BREAK() DebugBreak()
    #pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#else
    #define EZ_MSVC_WARN_PUSH(PP_NUMBER)
    #define EZ_MSVC_WARN_POP()
    #define EZ_DEBUG_BREAK() raise(SIGTRAP)
#endif
