// Copyright (C) 2025 Jonathan Müller and v:null contributors
// SPDX-License-Identifier: BSL-1.0

#include <cstdio>

#include <exy/future/factory.hpp>
#include <exy/future/transform.hpp>
#include <exy/sync_wait.hpp>

namespace exyf = exy::futures;

int main()
{
    auto pipeline = exyf::transform(exyf::value(11), [](int i) { return i * 2; });
    auto result   = exy::sync_wait(exy_mov(pipeline));
    std::printf("%d\n", result);
}

