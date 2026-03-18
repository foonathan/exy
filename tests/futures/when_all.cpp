// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/when_all.hpp>

#include <exy/future/factory.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEST_CASE("when_all", "[futures]")
{
    auto nullary = exyf::when_all();
    REQUIRE_SIGNATURES(nullary, exy::value_tag());
    REQUIRE_FUTURE(nullary, exy::value_tag());

    auto unary = exyf::when_all(exyf::value(0));
    REQUIRE_SIGNATURES(unary, exy::value_tag(int));
    REQUIRE_FUTURE(unary, exy::value_tag(), 0);

    auto unary_unary = exyf::when_all(exyf::value(0), exyf::value(1));
    REQUIRE_SIGNATURES(unary_unary, exy::value_tag(int, int));
    REQUIRE_FUTURE(unary_unary, exy::value_tag(), 0, 1);

    auto unary_unary_unary = exyf::when_all(exyf::value(0), exyf::value(1), exyf::value(2));
    REQUIRE_SIGNATURES(unary_unary_unary, exy::value_tag(int, int, int));
    REQUIRE_FUTURE(unary_unary_unary, exy::value_tag(), 0, 1, 2);

    auto unary_binary_unary = exyf::when_all(exyf::value(0), exyf::value(1, 2), exyf::value(3));
    REQUIRE_SIGNATURES(unary_binary_unary, exy::value_tag(int, int, int, int));
    REQUIRE_FUTURE(unary_binary_unary, exy::value_tag(), 0, 1, 2, 3);

    auto unary_nullary_unary = exyf::when_all(exyf::value(0), exyf::value(), exyf::value(1));
    REQUIRE_SIGNATURES(unary_nullary_unary, exy::value_tag(int, int));
    REQUIRE_FUTURE(unary_nullary_unary, exy::value_tag(), 0, 1);

    auto unary_error_unary = exyf::when_all(
        exyf::value(0), with_signature<exy::value_tag(int)>(exyf::error(-1)), exyf::value(2)
    );
    REQUIRE_SIGNATURES(unary_error_unary, exy::error_tag(int), exy::value_tag(int, int, int));
    REQUIRE_FUTURE(unary_error_unary, exy::error_tag(), -1);

    auto unary_stopped_unary = exyf::when_all(
        exyf::value(0), with_signature<exy::value_tag(int)>(exyf::stopped()), exyf::value(2)
    );
    REQUIRE_SIGNATURES(unary_stopped_unary, exy::stopped_tag(), exy::value_tag(int, int, int));
    REQUIRE_FUTURE(unary_stopped_unary, exy::stopped_tag());

    auto throwing_move = exyf::when_all(exyf::value(0), exyf::value(move_only(1)));
    REQUIRE_SIGNATURES(
        throwing_move, exy::error_tag(std::exception_ptr), exy::value_tag(int, move_only)
    );
    REQUIRE_FUTURE(exy_mov(throwing_move), exy::value_tag(), 0, move_only(1));
}

