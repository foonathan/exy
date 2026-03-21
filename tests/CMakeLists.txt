# Copyright (C) 2026 Jonathan Müller and exy contributors
# SPDX-License-Identifier: BSL-1.0

find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

add_executable(exy_tests
    futures/and_then.cpp
    futures/environment.cpp
    futures/factory.cpp
    futures/repeat.cpp
    futures/schedule.cpp
    futures/stop_point.cpp
    futures/transform.cpp
    futures/when_all.cpp
    futures/when_any.cpp
    futures/with.cpp
)
target_link_libraries(exy_tests PRIVATE exy_dev Catch2::Catch2WithMain)
target_compile_options(exy_tests PRIVATE -Wno-c2y-extensions)

catch_discover_tests(exy_tests)

