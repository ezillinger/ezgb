cmake_minimum_required(VERSION 3.14)

# set the project name
project( CoronaBoy )

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(EZ_ADDRESS_SANITIZER "Enable Clang address sanitiizer" False)
option(EZ_USE_FREETYPE "Use Freetype for font rendering" False)

if(MSVC)
    set(SDL2_DIR "C:\\git\\SDL2-2.30.3\\cmake\\")
endif()

add_subdirectory(src)