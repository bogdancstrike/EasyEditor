# Top-level CMake. Used standalone for desktop builds (tests, fuzzers) and
# also pulled in by the Android Gradle build via externalNativeBuild.
cmake_minimum_required(VERSION 3.22)
project(video_editor LANGUAGES CXX C VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Treat warnings as errors in CI; relaxed for IDE iteration via -DVX_WERROR=OFF.
option(VX_WERROR "Treat compiler warnings as errors" ON)
option(VX_BUILD_TESTS "Build unit tests" ON)
option(VX_ENABLE_SANITIZERS "Enable ASan + UBSan in debug" OFF)

if (VX_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
                        -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion)
    if (VX_WERROR)
        add_compile_options(-Werror)
    endif()
    if (VX_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

add_subdirectory(core)
