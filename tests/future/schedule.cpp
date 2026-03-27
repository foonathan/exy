// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/future/schedule.hpp>

#include <thread>
#include <exy/future/factory.hpp>
#include <exy/scheduler/inline.hpp>
#include <exy/scheduler/parallel.hpp>
#include "test.hpp"

namespace exyf = exy::futures;
namespace exys = exy::schedulers;

namespace
{
constexpr struct background_backend : exy::parallel_scheduler_backend
{
    void schedule(job& j, exy::state_base& s) const override
    {
        // For the test we don't really care whether it truly runs in the background, just as long
        // as it is a different thread.
        std::jthread([&] { j.continuation(s); });
    }
} background_backend;

const struct failing_backend : exy::parallel_scheduler_backend
{
    std::exception_ptr ex = std::make_exception_ptr(0);

    void schedule(job&, exy::state_base&) const override
    {
        std::rethrow_exception(ex);
    }
} failing_backend;
} // namespace

TEST_CASE("schedule", "[futures]")
{
    auto infallible = exyf::schedule(exys::inline_);
    REQUIRE_SIGNATURES(infallible, exy::value_tag());
    CHECK_FUTURE(infallible, no_thread_change, exy::value_tag());

    auto failing = exyf::schedule(exys::parallel(failing_backend));
    REQUIRE_SIGNATURES(failing, exy::value_tag(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(failing, no_thread_change, exy::error_tag(), auto(failing_backend.ex));

    auto background = exyf::schedule(exys::parallel(background_backend));
    REQUIRE_SIGNATURES(background, exy::value_tag(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(background, thread_change, exy::value_tag());
}

TEST_CASE("starts_on", "[futures]")
{
    auto infalliable = exyf::starts_on(exys::inline_, exyf::value(11));
    REQUIRE_SIGNATURES(infalliable, exy::value_tag(int));
    CHECK_FUTURE(infalliable, no_thread_change, exy::value_tag(), 11);

    auto failing = exyf::starts_on(exys::parallel(failing_backend), exyf::value(11));
    REQUIRE_SIGNATURES(failing, exy::error_tag(std::exception_ptr), exy::value_tag(int));
    CHECK_FUTURE(failing, no_thread_change, exy::error_tag(), auto(failing_backend.ex));

    auto background = exyf::starts_on(exys::parallel(background_backend), exyf::value(11));
    REQUIRE_SIGNATURES(background, exy::error_tag(std::exception_ptr), exy::value_tag(int));
    CHECK_FUTURE(background, thread_change, exy::value_tag(), 11);
}

TEST_CASE("continues_on", "[futures]")
{
    exys::parallel background(background_backend);

    auto nullary_value = exyf::value() | exyf::continues_on(background);
    REQUIRE_SIGNATURES(nullary_value, exy::value_tag(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(nullary_value, thread_change, exy::value_tag());

    auto unary_value = exyf::value(11) | exyf::continues_on(background);
    REQUIRE_SIGNATURES(unary_value, exy::value_tag(int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(unary_value, thread_change, exy::value_tag(), 11);

    auto binary_value = exyf::value(11, 42) | exyf::continues_on(background);
    REQUIRE_SIGNATURES(binary_value, exy::value_tag(int, int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(binary_value, thread_change, exy::value_tag(), 11, 42);

    auto multiple_values
        = with_signature<exy::value_tag(short)>(exyf::value(11)) | exyf::continues_on(background);
    REQUIRE_SIGNATURES(
        multiple_values, exy::value_tag(int), exy::value_tag(short),
        exy::error_tag(std::exception_ptr)
    );
    CHECK_FUTURE(multiple_values, thread_change, exy::value_tag(), 11);

    auto error = exyf::error(0) | exyf::continues_on(background);
    REQUIRE_SIGNATURES(error, exy::error_tag(int), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(error, no_thread_change, exy::error_tag(), 0);

    auto stopped = exyf::stopped() | exyf::continues_on(background);
    REQUIRE_SIGNATURES(stopped, exy::stopped_tag(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(stopped, no_thread_change, exy::stopped_tag());

    auto infallible = exyf::value() | exyf::continues_on(exys::inline_);
    REQUIRE_SIGNATURES(infallible, exy::value_tag());
    CHECK_FUTURE(infallible, exy::value_tag());

    auto failing = exyf::value() | exyf::continues_on(exys::parallel(failing_backend));
    REQUIRE_SIGNATURES(failing, exy::value_tag(), exy::error_tag(std::exception_ptr));
    CHECK_FUTURE(failing, no_thread_change, exy::error_tag(), auto(failing_backend.ex));
}

