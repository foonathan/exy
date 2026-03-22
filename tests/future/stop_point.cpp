// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/stop_point.hpp>

#include <exy/future/factory.hpp>
#include <exy/future/transform.hpp>
#include <exy/future/with.hpp>
#include "test.hpp"

namespace exyf = exy::futures;
namespace exyq = exy::queries;

namespace
{
struct stop_env_t : test_env_t
{
    bool stop_requested = false;

    using test_env_t::query;

    constexpr bool query(exyq::stop_requested_t) const noexcept
    {
        return stop_requested;
    }
};
} // namespace

TEST_CASE("stop_point in default environment", "[futures]")
{
    auto f = exyf::value(11) | exyf::stop_point;
    REQUIRE_SIGNATURES(f, exy::value_tag(int), exy::stopped_tag());
    CHECK_FUTURE(f, exy::value_tag(), 11);
}

TEST_CASE("stop_point in stop_env", "[futures]")
{
    stop_env_t test_env;
    auto do_stop = exyf::with([&](const auto&...) noexcept { test_env.stop_requested = true; });

    test_env.stop_requested = false;
    auto no_stop            = exyf::value(11) | exyf::stop_point;
    REQUIRE_SIGNATURES(no_stop, exy::value_tag(int), exy::stopped_tag());
    CHECK_FUTURE(no_stop, exy::value_tag(), 11);

    test_env.stop_requested = false;
    auto stop               = exyf::value(11) | do_stop | exyf::stop_point
                            | exyf::transform([](int) noexcept { FAIL("unreachable"); });
    REQUIRE_SIGNATURES(stop, exy::value_tag(), exy::stopped_tag());
    CHECK_FUTURE(stop, exy::stopped_tag());
}

