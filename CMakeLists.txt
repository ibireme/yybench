cmake_minimum_required (VERSION 3.5)
project (yybench VERSION 1.0.0)

# Options
option(YYBENCH_BUILD_TESTS "Build tests" OFF)


# Build Type
if (XCODE OR MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to: Release")
    set(CMAKE_BUILD_TYPE Release)
endif()


# Library
file(GLOB SOURCES "src/*.h" "src/*.c")
add_library(yybench ${SOURCES})
target_include_directories(yybench PUBLIC src)

# Tests
if(YYBENCH_BUILD_TESTS)
    add_executable(yybench_test "test/yybench_test.c")
    target_link_libraries(yybench_test yybench)
endif()

# Project Config
if(XCODE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(XcodeProperty)

    set_default_xcode_property(yybench)
    set_xcode_deployment_version(yybench "10.11" "9.0" "9.0" "2.0")
    if(YYBENCH_BUILD_TESTS)
        set_default_xcode_property(yybench_test)
    endif()
endif()
