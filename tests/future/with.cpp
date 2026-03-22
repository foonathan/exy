// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/with.hpp>

#include <exy/future/factory.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEMPLATE_TEST_CASE("with", "[futures]", exy::value_tag, exy::error_tag, exy::stopped_tag)
{
    constexpr auto factory = exyf::factory_t<TestType>{};
    constexpr auto with    = exyf::with_t<TestType>{};

    auto nullary = factory() | with([]() noexcept {});
    REQUIRE_SIGNATURES(nullary, TestType());
    CHECK_FUTURE(nullary, TestType());

    auto unary = factory(42) | with([](int x) noexcept { REQUIRE(x == 42); });
    REQUIRE_SIGNATURES(unary, TestType(int));
    CHECK_FUTURE(unary, TestType(), 42);

    auto binary = factory(42, 11) | with([](int a, int b) noexcept {
                      REQUIRE(a == 42);
                      REQUIRE(b == 11);
                  });
    REQUIRE_SIGNATURES(binary, TestType(int, int));
    CHECK_FUTURE(binary, TestType(), 42, 11);

    auto throwing_fn = factory() | with([] {});
    REQUIRE_SIGNATURES(throwing_fn, TestType(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(throwing_fn, TestType());
}

