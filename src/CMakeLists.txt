file(GLOB SOURCES
    ./*.cpp
)

add_executable(ezgb 
              ${SOURCES}
)

target_compile_definitions(ezgb PRIVATE IMGUI_USER_CONFIG="../../ImGuiConfig.h")

target_include_directories(ezgb PRIVATE 
                          ./libs/imgui 
)   

# some xcode versions have a bad linker - use the old one
if (APPLE)
  message("Using classic macOS linker")
  set_property(TARGET ezgb PROPERTY LINKER_TYPE APPLE_CLASSIC)
endif()

if(MSVC)
  target_compile_options(ezgb PRIVATE /W4 /WX)
  target_compile_features(ezgb PRIVATE cxx_std_20)
  set_target_properties(ezgb PROPERTIES WIN32_EXECUTABLE TRUE)

   add_custom_command(
        TARGET ezgb POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:ezgb>"
        VERBATIM
    )
else()
    # Clang and GCC compatible stuff here
  target_compile_options(ezgb PRIVATE 
                          -Wall 
                          -Wextra 
                          #-Wpedantic 
                          -Werror)
  
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message("Clang Detected!")
    if(EZ_ADDRESS_SANITIZER)
      target_compile_options(ezgb PRIVATE "-fsanitize=address")
      target_link_options(ezgb PRIVATE "-fsanitize=address")
    endif()
    # Clang specific stuff here
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message("GCC Detected!")
    # GCC specific stuff here
  endif()


find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(ezgb SDL2::SDL2 OpenGL::GL)

if(EZ_USE_FREETYPE)
  find_package(Freetype REQUIRED)
  target_link_libraries(ezgb Freetype::Freetype)
  target_include_directories(ezgb PRIVATE ./libs/imgui/misc/freetype)
  target_sources(ezgb PRIVATE ./libs/imgui/misc/freetype/imgui_freetype.cpp)
endif()