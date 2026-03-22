# Copyright (C) 2026 Jonathan Müller and exy contributors
# SPDX-License-Identifier: BSL-1.0

find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

add_executable(exy_tests
    future/and_then.cpp
    future/environment.cpp
    future/factory.cpp
    future/repeat.cpp
    future/schedule.cpp
    future/stop_point.cpp
    future/transform.cpp
    future/when_all.cpp
    future/when_any.cpp
    future/with.cpp
    scheduler/defer.cpp
)
target_link_libraries(exy_tests PRIVATE exy_dev Catch2::Catch2WithMain)
target_compile_options(exy_tests PRIVATE -Wno-c2y-extensions)

catch_discover_tests(exy_tests)

