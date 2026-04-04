// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/future/repeat.hpp>

#include <exy/future/factory.hpp>
#include <exy/future/transform.hpp>
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

TEST_CASE("repeat in default environment", "[futures]")
{
    auto failing = with_signature<exy::value_tag()>(exyf::error(0)) | exyf::repeat;
    REQUIRE_SIGNATURES(
        failing, exy::error_tag(int), exy::error_tag(std::exception_ptr), exy::stopped_tag()
    );
    CHECK_FUTURE(failing, exy::error_tag(), 0);
}

TEST_CASE("repeat in stop_env", "[futures]")
{
    stop_env_t test_env;
    int        count = 0;

    auto nothrow = exyf::value() | exyf::transform([&] noexcept {
                       if (++count == 3)
                           test_env.stop_requested = true;
                   })
                 | exyf::repeat;
    REQUIRE_SIGNATURES(nothrow, exy::error_tag(std::exception_ptr), exy::stopped_tag());
    CHECK_FUTURE(nothrow, exy::stopped_tag());

    test_env.stop_requested = false;
    auto failing            = with_signature<exy::value_tag()>(exyf::error(0)) | exyf::repeat;
    REQUIRE_SIGNATURES(
        failing, exy::error_tag(int), exy::error_tag(std::exception_ptr), exy::stopped_tag()
    );
    CHECK_FUTURE(failing, exy::error_tag(), 0);
}

