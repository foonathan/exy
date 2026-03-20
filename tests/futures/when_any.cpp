// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/when_any.hpp>

#include <exy/future/factory.hpp>
#include <exy/future/stop_point.hpp>
#include <exy/future/transform.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEST_CASE("when_any", "[futures]")
{
    auto value = exyf::when_any(exyf::value(0));
    REQUIRE_SIGNATURES(value, exy::value_tag(int));
    REQUIRE_FUTURE(value, exy::value_tag(), 0);

    auto value_value = exyf::when_any(exyf::value(0), exyf::value());
    REQUIRE_SIGNATURES(value_value, exy::value_tag(int), exy::value_tag());
    REQUIRE_FUTURE(value_value, exy::value_tag(), 0);

    auto error_value = exyf::when_any(exyf::error(-1), exyf::value(0));
    REQUIRE_SIGNATURES(error_value, exy::error_tag(int), exy::value_tag(int));
    REQUIRE_FUTURE(error_value, exy::error_tag(), -1);

    auto stop_value = exyf::when_any(
        exyf::value(0),
        exyf::value() | exyf::stop_point | exyf::transform([]() noexcept { FAIL("unreachable"); })
    );
    REQUIRE_SIGNATURES(stop_value, exy::value_tag(int), exy::value_tag(), exy::stopped_tag());
    REQUIRE_FUTURE(stop_value, exy::value_tag(), 0);

    auto throwing_move = exyf::when_any(exyf::value(0), exyf::value(move_only(1)));
    REQUIRE_SIGNATURES(
        throwing_move, exy::value_tag(int), exy::value_tag(move_only),
        exy::error_tag(std::exception_ptr)
    );
    REQUIRE_FUTURE(exy_mov(throwing_move), exy::value_tag(), 0);

    auto stop_on_error = exyf::when_any(
        exyf::error(0),
        exyf::value() | exyf::stop_point | exyf::transform([]() noexcept { FAIL("unreachable"); })
    );
    REQUIRE_SIGNATURES(stop_on_error, exy::error_tag(int), exy::value_tag(), exy::stopped_tag());
    REQUIRE_FUTURE(stop_on_error, exy::error_tag(), 0);

    auto stop_on_stopped = exyf::when_any(
        exyf::stopped(),
        exyf::value() | exyf::stop_point | exyf::transform([]() noexcept { FAIL("unreachable"); })
    );
    REQUIRE_SIGNATURES(stop_on_stopped, exy::stopped_tag(), exy::value_tag());
    REQUIRE_FUTURE(stop_on_stopped, exy::stopped_tag());
}

