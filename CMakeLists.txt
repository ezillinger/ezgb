cmake_minimum_required(VERSION 3.29)

# set the project name
project( ezgb )

# for some reason emscripten tries to link to the system libc++ if we use CMAKE_CXX_STANDARD
if(NOT EMSCRIPTEN)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
else()
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

option(EZ_ADDRESS_SANITIZER "Enable Clang address sanitiizer" False)
option(EZ_USE_FREETYPE "Use Freetype for font rendering" False)

if(MSVC AND NOT DEFINED SDL2_DIR)
    set(SDL2_DIR "C:\\git\\SDL2-2.30.3\\cmake\\")
endif()

file(GLOB SOURCES
    ./src/*.cpp
)

add_executable(ezgb ${SOURCES})

target_compile_definitions(ezgb PRIVATE IMGUI_USER_CONFIG="../../ImGuiConfig.h")

target_include_directories(ezgb PRIVATE 
                          ./src/libs/imgui 
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

if(NOT EMSCRIPTEN)
  target_link_libraries(ezgb SDL2::SDL2 OpenGL::GL)
else()
  # for some reason setting set(CMAKE_CXX_STANDARD 20) chooses the wrong version
  target_compile_options(ezgb PRIVATE 
    "-std=c++20" 
    "-sUSE_SDL=2"
  )
  target_link_options(ezgb PRIVATE 
    "-sUSE_SDL=2" 
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=0"
    "-sASSERTIONS=1"
  )

  add_custom_command(
        TARGET ezgb POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "../data/index.html" "$<TARGET_FILE_DIR:ezgb>"
        VERBATIM
    )
endif()

if(EZ_USE_FREETYPE)
  find_package(Freetype REQUIRED)
  target_link_libraries(ezgb Freetype::Freetype)
  target_include_directories(ezgb PRIVATE ./libs/imgui/misc/freetype)
  target_sources(ezgb PRIVATE ./src/libs/imgui/misc/freetype/imgui_freetype.cpp)
endif()