# Copyright (C) 2025 Jonathan Müller and coex contributors
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.0)
project(exy LANGUAGES CXX)

find_package(Boost REQUIRED)

function(add_alias target)
    add_library(foonathan::${target} ALIAS ${target})
    set_target_properties(${target} PROPERTIES EXPORT_NAME ${target})
endfunction()

# Base target for common options.
add_library(_exy_base INTERFACE)
target_compile_features(_exy_base INTERFACE cxx_std_26)
target_include_directories(_exy_base SYSTEM INTERFACE include)

# Core library.
add_library(exy INTERFACE)
add_alias(exy)
target_link_libraries(exy INTERFACE _exy_base boost::mp11)
target_include_directories(exy SYSTEM INTERFACE include)

# Core library with warnings; for development only.
add_library(exy_dev INTERFACE)
add_alias(exy_dev)
target_link_libraries(exy_dev INTERFACE _exy_base)
target_include_directories(exy_dev INTERFACE include)
target_compile_options(exy_dev INTERFACE -pedantic-errors -Werror -Wall -Wextra -Wconversion -Wsign-conversion)

# Temporary test target.
add_executable(exy_prototype prototype.cpp)
target_link_libraries(exy_prototype PRIVATE exy_dev)

enable_testing()
add_subdirectory(tests)

