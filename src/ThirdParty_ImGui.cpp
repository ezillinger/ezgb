#include "Base.h" 

EZ_MSVC_WARN_PUSH()
EZ_MSVC_WARN_DISABLE(4701)

EZ_GCC_WARN_PUSH()
EZ_GCC_WARN_DISABLE(-Wmaybe-uninitialized)

#include "libs/imgui/imgui.cpp"
#include "libs/imgui/imgui_demo.cpp"
#include "libs/imgui/imgui_draw.cpp"
#include "libs/imgui/imgui_tables.cpp"
#include "libs/imgui/imgui_widgets.cpp"
#include "libs/imgui/backends/imgui_impl_sdl2.cpp"
#include "libs/imgui/backends/imgui_impl_opengl3.cpp"

EZ_GCC_WARN_POP()
EZ_MSVC_WARN_POP()