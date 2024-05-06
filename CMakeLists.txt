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

add_subdirectory(src)