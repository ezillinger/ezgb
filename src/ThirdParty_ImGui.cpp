#include "Base.h" 

EZ_MSVC_WARN_PUSH()
EZ_MSVC_WARN_DISABLE(4701)

EZ_CLANG_GCC_WARN_PUSH()
#if EZ_GCC
EZ_CLANG_GCC_WARN_DISABLE("-Wmaybe-uninitialized")
#endif

#include "libs/imgui/imgui.cpp"
#include "libs/imgui/imgui_demo.cpp"
#include "libs/imgui/imgui_draw.cpp"
#include "libs/imgui/imgui_tables.cpp"
#include "libs/imgui/imgui_widgets.cpp"
#include "libs/imgui/backends/imgui_impl_sdl2.cpp"
#include "libs/imgui/backends/imgui_impl_opengl3.cpp"

EZ_CLANG_GCC_WARN_POP()
EZ_MSVC_WARN_POP()