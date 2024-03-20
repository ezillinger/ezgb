file(GLOB SOURCES
    ./*.h
    ./*.cpp
)

add_executable(CoronaBoy ${SOURCES})

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