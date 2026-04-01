cmake_minimum_required(VERSION 3.20)
project(cpp-fxprocessor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json — clangd needs this
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find system libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK   REQUIRED IMPORTED_TARGET jack)
pkg_check_modules(LILV   REQUIRED IMPORTED_TARGET lilv-0)
pkg_check_modules(LV2    REQUIRED IMPORTED_TARGET lv2)

add_executable(cpp-fxprocessor
    src/main.cpp
)

target_include_directories(cpp-fxprocessor PRIVATE
    include/   # for vendored clap headers
)

target_link_libraries(cpp-fxprocessor PRIVATE
    PkgConfig::JACK
    PkgConfig::LILV
    PkgConfig::LV2
    dl             # for dlopen — needed for CLAP
)

# Sanitizers for debug builds
target_compile_options(cpp-fxprocessor PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=undefined -g>
)
target_link_options(cpp-fxprocessor PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=undefined>
)