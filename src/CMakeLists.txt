file(GLOB SOURCES
    ./*.h
    ./*.cpp
)

add_executable(CoronaBoy ${SOURCES})

target_compile_features(CoronaBoy PRIVATE cxx_std_17)

target_link_libraries(CoronaBoy PRIVATE fmt::fmt spdlog::spdlog)