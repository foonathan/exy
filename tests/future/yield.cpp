// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/yield.hpp>

#include <exy/future/factory.hpp>
#include "test.hpp"

namespace exyf = exy::futures;
namespace exys = exy::schedulers;

namespace
{
struct yield_env_t : test_env_t
{
    using test_env_t::query;

    static constexpr auto query(exy::queries::delegation_scheduler_t) noexcept
    {
        return exys::parallel</*Noexcept=*/true>(background_backend);
    }
};
} // namespace

TEST_CASE("yield in default environment", "[futures]")
{
    auto f = exyf::value(11) | exyf::yield;
    REQUIRE_SIGNATURES(f, exy::value_tag(int));
    CHECK_FUTURE(f, no_thread_change, exy::value_tag(), 11);
}

TEST_CASE("yield in yield_env", "[futures]")
{
    yield_env_t test_env;

    auto f = exyf::value(11) | exyf::yield;
    REQUIRE_SIGNATURES(f, exy::value_tag(int));
    CHECK_FUTURE(f, thread_change, exy::value_tag(), 11);
}

