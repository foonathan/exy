// Copyright (C) 2025 Jonathan Müller and v:null contributors
// SPDX-License-Identifier: BSL-1.0

#include <cstdio>

#include <exy/future/and_then.hpp>
#include <exy/future/factory.hpp>
#include <exy/future/transform.hpp>
#include <exy/sync_wait.hpp>

namespace exyf = exy::futures;

int main()
{
    auto pipeline = exyf::value(11, 42) | exyf::transform([](int a, int b) { return a + b; })
                  | exyf::and_then([](int x) { return exyf::value(x); })
                  | exyf::upon_error([](std::exception_ptr&&) { return 42; })
                  | exyf::upon_stopped([]() { return -1; });
    auto [result] = *exy::sync_wait(exy_mov(pipeline));
    std::printf("%d\n", result);
}

