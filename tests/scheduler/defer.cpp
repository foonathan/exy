// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/scheduler/defer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <exy/future/factory.hpp>
#include <exy/future/schedule.hpp>
#include <exy/future/transform.hpp>
#include <exy/future/when_all.hpp>
#include <exy/sync_wait.hpp>

namespace exyf = exy::futures;
namespace exys = exy::schedulers;

TEST_CASE("defer", "[schedulers]")
{
    exy::run_loop loop;

    int i = 0;
    exy::sync_wait(
        exyf::when_all(
            exyf::schedule(exys::defer(loop)) | exyf::transform([&] noexcept { CHECK(i++ == 0); }),
            exyf::schedule(exys::defer(loop)) | exyf::transform([&] noexcept { CHECK(i++ == 1); }),
            exyf::schedule(exys::defer(loop)) | exyf::transform([&] noexcept { CHECK(i++ == 2); }),
            exyf::value() | exyf::transform([&] noexcept {
                CHECK(i == 0);
                loop.finish();
                loop.run();
            })
        )
    );
    CHECK(i == 3);
}

