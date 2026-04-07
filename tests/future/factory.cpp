// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/future/factory.hpp>

#include "test.hpp"

namespace exyf = exy::futures;

TEMPLATE_TEST_CASE("factory", "[futures]", exy::value_tag, exy::error_tag, exy::stopped_tag)
{
    constexpr auto f = exyf::factory_t<TestType>{};

    REQUIRE_SIGNATURES(f(), TestType());
    CHECK_FUTURE(f(), TestType());

    REQUIRE_SIGNATURES(f(11), TestType(int));
    CHECK_FUTURE(f(11), TestType(), 11);

    REQUIRE_SIGNATURES(f(11, 42), TestType(int, int));
    CHECK_FUTURE(f(11, 42), TestType(), 11, 42);

    REQUIRE_SIGNATURES(f(move_only(17)), TestType(move_only), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(f(move_only(17)), TestType(), move_only(17));
}

TEST_CASE("run", "[futures]")
{
    auto nothrow = exyf::run([] noexcept { return 11; });
    REQUIRE_SIGNATURES(nothrow, exy::value_tag(int));
    CHECK_FUTURE(nothrow, exy::value_tag(), 11);

    auto throwing = exyf::run([] { return 11; });
    REQUIRE_SIGNATURES(throwing, exy::value_tag(int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(throwing, exy::value_tag(), 11);

    auto throwing_move = exyf::run([] { return move_only(11); });
    REQUIRE_SIGNATURES(
        throwing_move, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    CHECK_FUTURE(throwing_move, exy::value_tag(), move_only(11));
}

