// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/and_then.hpp>

#include <exy/future/factory.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEMPLATE_TEST_CASE("and_then", "[futures]", exy::value_tag, exy::error_tag, exy::stopped_tag)
{
    constexpr auto factory  = exyf::factory_t<TestType>{};
    constexpr auto and_then = exyf::and_then_t<TestType>{};

    auto to_value_int = factory() | and_then([] noexcept { return exyf::value(42); });
    REQUIRE_SIGNATURES(to_value_int, exy::value_tag(int));
    CHECK_FUTURE(to_value_int, exy::value_tag(), 42);

    auto to_error_int = factory() | and_then([] noexcept { return exyf::error(42); });
    REQUIRE_SIGNATURES(to_error_int, exy::error_tag(int));
    CHECK_FUTURE(to_error_int, exy::error_tag(), 42);

    auto to_stopped_int = factory() | and_then([] noexcept { return exyf::stopped(42); });
    REQUIRE_SIGNATURES(to_stopped_int, exy::stopped_tag(int));
    CHECK_FUTURE(to_stopped_int, exy::stopped_tag(), 42);

    auto unary = factory(11) | and_then([](int x) noexcept { return exyf::value(x + 1); });
    REQUIRE_SIGNATURES(unary, exy::value_tag(int));
    CHECK_FUTURE(unary, exy::value_tag(), 12);

    auto binary
        = factory(11, 42) | and_then([](int a, int b) noexcept { return exyf::value(a + b); });
    REQUIRE_SIGNATURES(binary, exy::value_tag(int));
    CHECK_FUTURE(binary, exy::value_tag(), 53);

    auto throwing_fn = factory() | and_then([] { return exyf::value(42); });
    REQUIRE_SIGNATURES(throwing_fn, exy::value_tag(int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(throwing_fn, exy::value_tag(), 42);

    auto throwing_state = factory() | and_then([] noexcept { return exyf::value(move_only(42)); });
    REQUIRE_SIGNATURES(
        throwing_state, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    CHECK_FUTURE(throwing_state, exy::value_tag(), move_only(42));
}

