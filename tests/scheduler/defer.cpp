// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <exy/scheduler/defer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <exy/detach.hpp>
#include <exy/future/schedule.hpp>
#include <exy/future/transform.hpp>

namespace exyf = exy::futures;
namespace exys = exy::schedulers;

TEST_CASE("defer", "[schedulers]")
{
    exy::run_loop loop;

    int i = 0;
    exy::detach(exyf::schedule(exys::defer(loop)) | exyf::transform([&] { CHECK(i++ == 0); }));
    exy::detach(exyf::schedule(exys::defer(loop)) | exyf::transform([&] { CHECK(i++ == 1); }));
    exy::detach(exyf::schedule(exys::defer(loop)) | exyf::transform([&] { CHECK(i++ == 2); }));
    CHECK(i == 0);

    loop.finish();
    loop.run();
    CHECK(i == 3);
}

