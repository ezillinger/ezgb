file(GLOB SOURCES
    ./*.cpp
)

file(GLOB IMGUI_SOURCES
    ./libs/imgui/*.cpp
    ./libs/imgui/backends/imgui_impl_sdl2.cpp
    ./libs/imgui/backends/imgui_impl_opengl3.cpp
    ./libs/imgui/misc/freetype/imgui_freetype.cpp
)

add_executable(CoronaBoy 
              ${SOURCES}
              ${IMGUI_SOURCES}
)

target_compile_definitions(CoronaBoy PRIVATE IMGUI_USER_CONFIG="../../ImGuiConfig.h")

target_include_directories(CoronaBoy PRIVATE 
                          ./libs/imgui 
#                          ./libs/imgui/misc/freetype
)   

if(MSVC)
  target_compile_options(CoronaBoy PRIVATE /W4 /WX)
  target_compile_features(CoronaBoy PRIVATE cxx_std_20)
else()
  target_compile_options(CoronaBoy PRIVATE 
                          -Wall 
                          -Wextra 
                          #-Wpedantic 
                          -Werror)
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
target_link_libraries(CoronaBoy SDL2::SDL2 OpenGL::GL Freetype::Freetype)