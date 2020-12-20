cmake_minimum_required (VERSION 3.5)
project (yybench VERSION 1.0.0)


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

# Project Config
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XcodeProperty)

if(XCODE)
    set_default_xcode_property(yybench)
    set_xcode_deployment_version(yybench "10.11" "9.0" "9.0" "2.0")

    set_xcode_property(yybench GCC_C_LANGUAGE_STANDARD "c99")
    set_xcode_property(yybench CLANG_CXX_LANGUAGE_STANDARD "c++11")
    
    set_default_xcode_property(yybench_test)
elseif(MSVC)
    target_compile_options(yybench PRIVATE $<$<C_COMPILER_ID:MSVC>:/permissive- /utf-8>)
    target_compile_options(yybench PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/permissive- /utf-8>)
endif()