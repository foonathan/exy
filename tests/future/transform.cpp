// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/future/transform.hpp>

#include <exy/future/factory.hpp>
#include "test.hpp"

namespace exyf = exy::futures;

TEMPLATE_TEST_CASE(
    "transform", "[futures]", (std::tuple<exy::value_tag, exy::value_tag>),
    (std::tuple<exy::error_tag, exy::error_tag>), (std::tuple<exy::stopped_tag, exy::stopped_tag>),
    (std::tuple<exy::error_tag, exy::value_tag>), (std::tuple<exy::stopped_tag, exy::value_tag>)
)
{
    using from_tag = std::tuple_element_t<0, TestType>;
    using to_tag   = std::tuple_element_t<1, TestType>;

    constexpr auto factory   = exyf::factory_t<from_tag>{};
    constexpr auto transform = exyf::transform_t<from_tag, to_tag>{};

    auto nullary_to_int = factory() | transform([] noexcept { return 0; });
    REQUIRE_SIGNATURES(nullary_to_int, to_tag(int));
    CHECK_FUTURE(nullary_to_int, to_tag(), 0);

    auto unary_to_int = factory(11) | transform([](int x) noexcept { return x; });
    REQUIRE_SIGNATURES(unary_to_int, to_tag(int));
    CHECK_FUTURE(unary_to_int, to_tag(), 11);

    auto binary_to_int = factory(11, 42) | transform([](int a, int b) noexcept { return a + b; });
    REQUIRE_SIGNATURES(binary_to_int, to_tag(int));
    CHECK_FUTURE(binary_to_int, to_tag(), 53);

    auto to_void = factory() | transform([]() noexcept {});
    REQUIRE_SIGNATURES(to_void, to_tag());
    CHECK_FUTURE(to_void, to_tag());

    auto throwing_fn = factory() | transform([] { return 0; });
    REQUIRE_SIGNATURES(throwing_fn, to_tag(int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(throwing_fn, to_tag(), 0);

    auto throwing_move_result = factory() | transform([] noexcept { return move_only(0); });
    REQUIRE_SIGNATURES(throwing_move_result, to_tag(move_only), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(throwing_move_result, to_tag(), move_only(0));
}

