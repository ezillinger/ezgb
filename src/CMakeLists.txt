file(GLOB SOURCES
    ./*.cpp
)

add_executable(CoronaBoy 
              ${SOURCES}
)

target_compile_definitions(CoronaBoy PRIVATE IMGUI_USER_CONFIG="../../ImGuiConfig.h")

target_include_directories(CoronaBoy PRIVATE 
                          ./libs/imgui 
#                          ./libs/imgui/misc/freetype
)   

if(MSVC)
  target_compile_options(CoronaBoy PRIVATE /W4 /WX)
  target_compile_features(CoronaBoy PRIVATE cxx_std_20)
  set_target_properties(CoronaBoy PROPERTIES WIN32_EXECUTABLE TRUE)

   add_custom_command(
        TARGET CoronaBoy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:CoronaBoy>"
        VERBATIM
    )
else()
    # Clang and GCC compatible stuff here
  target_compile_options(CoronaBoy PRIVATE 
                          -Wall 
                          -Wextra 
                          #-Wpedantic 
                          -Werror)
  
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message("Clang Detected!")
    if(EZ_ADDRESS_SANITIZER)
      target_compile_options(CoronaBoy PRIVATE "-fsanitize=address")
      target_link_options(CoronaBoy PRIVATE "-fsanitize=address")
    endif()
    # Clang specific stuff here
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message("GCC Detected!")
    # GCC specific stuff here
  endif()


find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(CoronaBoy SDL2::SDL2 OpenGL::GL)

if(EZ_USE_FREETYPE)
  find_package(Freetype REQUIRED)
  target_link_libraries(CoronaBoy Freetype::Freetype)
  target_include_directories(CoronaBoy PRIVATE ./libs/imgui/misc/freetype)
  target_sources(CoronaBoy PRIVATE ./libs/imgui/misc/freetype/imgui_freetype.cpp)
endif()