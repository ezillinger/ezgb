#pragma once

#if defined(__clang__)
    #define EZ_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
    #define EZ_GCC 1
#elif defined(_MSC_VER)
    #define EZ_MSVC 1
    // disable polluting #defines
    #define NOGDI
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN 1
    #define _CRT_SECURE_NO_WARNINGS
    #include <debugapi.h>
    #include <windows.h>
#endif

#if EZ_MSVC

    #define EZ_DO_PRAGMA(PP_VALUE) __pragma(#PP_VALUE)
    #define EZ_MSVC_WARN_PUSH() __pragma(warning(push))
    #define EZ_MSVC_WARN_POP() __pragma(warning(pop))
    #define EZ_MSVC_WARN_DISABLE(PP_ERROR_NO) __pragma(warning(disable : PP_ERROR_NO))
    #define EZ_DEBUG_BREAK() DebugBreak()

EZ_MSVC_WARN_DISABLE(4201) // we're using anonymous structs/unions extensively
#else
    #define EZ_DO_PRAGMA(PP_VALUE) _Pragma(#PP_VALUE)
    #define EZ_MSVC_WARN_PUSH()
    #define EZ_MSVC_WARN_POP()
    #define EZ_MSVC_WARN_DISABLE(PP_ERROR_NO)
    #define EZ_DEBUG_BREAK() raise(SIGTRAP)

#endif

#if EZ_GCC
    #define EZ_CLANG_GCC_WARN_PUSH() _Pragma("GCC diagnostic push")
    #define EZ_CLANG_GCC_WARN_POP() _Pragma("GCC diagnostic pop")
    #define EZ_CLANG_GCC_WARN_DISABLE(PP_WARNING) EZ_DO_PRAGMA(GCC diagnostic ignored #PP_WARNING)
#elif EZ_CLANG
    #define EZ_CLANG_GCC_WARN_PUSH() _Pragma("clang diagnostic push")
    #define EZ_CLANG_GCC_WARN_POP() _Pragma("clang diagnostic pop")
    #define EZ_CLANG_GCC_WARN_DISABLE(PP_WARNING) EZ_DO_PRAGMA(clang diagnostic ignored #PP_WARNING)
#else
    #define EZ_CLANG_GCC_WARN_PUSH()
    #define EZ_CLANG_GCC_WARN_POP()
    #define EZ_CLANG_GCC_WARN_DISABLE(PP_WARNING)
#endif

EZ_CLANG_GCC_WARN_DISABLE(-Wformat - security)