// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/factory.hpp>

#include "test.hpp"

namespace exyf = exy::futures;

TEMPLATE_TEST_CASE("factory", "[futures]", exy::value_tag, exy::error_tag, exy::stopped_tag)
{
    constexpr auto f = exyf::factory_t<TestType>{};

    REQUIRE_SIGNATURES(f(), TestType());
    REQUIRE_FUTURE(f(), TestType());

    REQUIRE_SIGNATURES(f(11), TestType(int));
    REQUIRE_FUTURE(f(11), TestType(), 11);

    REQUIRE_SIGNATURES(f(11, 42), TestType(int, int));
    REQUIRE_FUTURE(f(11, 42), TestType(), 11, 42);

    REQUIRE_SIGNATURES(f(move_only(17)), TestType(move_only), exy::error_tag(std::exception_ptr));
    REQUIRE_FUTURE(f(move_only(17)), TestType(), move_only(17));
}

