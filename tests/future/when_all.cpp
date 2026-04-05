// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/future/when_all.hpp>

#include <exy/future/factory.hpp>
#include <exy/future/stop_point.hpp>
#include <exy/future/transform.hpp>
#include <exy/future/yield.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEST_CASE("when_all", "[futures]")
{
    auto unary = exyf::when_all(exyf::value(0));
    REQUIRE_SIGNATURES(unary, exy::value_tag(int));
    CHECK_FUTURE(unary, exy::value_tag(), 0);

    auto unary_unary = exyf::when_all(exyf::value(0), exyf::value(1));
    REQUIRE_SIGNATURES(unary_unary, exy::value_tag(int, int));
    CHECK_FUTURE(unary_unary, exy::value_tag(), 0, 1);

    auto unary_unary_unary = exyf::when_all(exyf::value(0), exyf::value(1), exyf::value(2));
    REQUIRE_SIGNATURES(unary_unary_unary, exy::value_tag(int, int, int));
    CHECK_FUTURE(unary_unary_unary, exy::value_tag(), 0, 1, 2);

    auto unary_binary_unary = exyf::when_all(exyf::value(0), exyf::value(1, 2), exyf::value(3));
    REQUIRE_SIGNATURES(unary_binary_unary, exy::value_tag(int, int, int, int));
    CHECK_FUTURE(unary_binary_unary, exy::value_tag(), 0, 1, 2, 3);

    auto unary_nullary_unary = exyf::when_all(exyf::value(0), exyf::value(), exyf::value(1));
    REQUIRE_SIGNATURES(unary_nullary_unary, exy::value_tag(int, int));
    CHECK_FUTURE(unary_nullary_unary, exy::value_tag(), 0, 1);

    auto unary_error_unary = exyf::when_all(
        exyf::value(0), with_signature<exy::value_tag(int)>(exyf::error(-1)), exyf::value(2)
    );
    REQUIRE_SIGNATURES(unary_error_unary, exy::error_tag(int), exy::value_tag(int, int, int));
    CHECK_FUTURE(unary_error_unary, exy::error_tag(), -1);

    auto unary_stopped_unary = exyf::when_all(
        exyf::value(0), with_signature<exy::value_tag(int)>(exyf::stopped()), exyf::value(2)
    );
    REQUIRE_SIGNATURES(unary_stopped_unary, exy::stopped_tag(), exy::value_tag(int, int, int));
    CHECK_FUTURE(unary_stopped_unary, exy::stopped_tag());

    auto throwing_move = exyf::when_all(exyf::value(0), exyf::value(move_only(1)));
    REQUIRE_SIGNATURES(
        throwing_move, exy::error_tag(std::exception_ptr), exy::value_tag(int, move_only)
    );
    CHECK_FUTURE(exy_mov(throwing_move), exy::value_tag(), 0, move_only(1));

    auto stop_on_error = exyf::when_all(
        exyf::value(), with_signature<exy::value_tag()>(exyf::error(11)),
        exyf::value() | exyf::stop_point | exyf::transform([]() noexcept { FAIL("unreachable"); })
    );
    REQUIRE_SIGNATURES(stop_on_error, exy::error_tag(int), exy::stopped_tag(), exy::value_tag());
    CHECK_FUTURE(stop_on_error, exy::error_tag(), 11);

    auto stop_on_stop = exyf::when_all(
        exyf::value(), with_signature<exy::value_tag()>(exyf::stopped(11)),
        exyf::value() | exyf::stop_point | exyf::transform([]() noexcept { FAIL("unreachable"); })
    );
    REQUIRE_SIGNATURES(stop_on_stop, exy::stopped_tag(int), exy::stopped_tag(), exy::value_tag());
    CHECK_FUTURE(stop_on_stop, exy::stopped_tag(), 11);

    auto counter      = 0;
    auto interleaving = exyf::when_all(
        exyf::value() | exyf::transform([&] noexcept { CHECK(counter++ == 0); }) | exyf::yield
            | exyf::transform([&] noexcept {
                  CHECK(counter++ == 3);
                  return counter;
              }),
        exyf::value() | exyf::transform([&] noexcept { CHECK(counter++ == 1); }) | exyf::yield
            | exyf::yield | exyf::transform([&]() noexcept {
                  CHECK(counter++ == 5);
                  return counter;
              }),
        exyf::value() | exyf::transform([&] noexcept { CHECK(counter++ == 2); }) | exyf::yield
            | exyf::transform([&]() noexcept {
                  CHECK(counter++ == 4);
                  return counter;
              })
    );
    REQUIRE_SIGNATURES(interleaving, exy::value_tag(int, int, int));
    CHECK_FUTURE(interleaving, exy::value_tag(), 4, 6, 5);
}

