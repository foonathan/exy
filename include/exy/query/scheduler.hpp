// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_QUERY_SCHEDULER_HPP_INCLUDED
#define EXY_QUERY_SCHEDULER_HPP_INCLUDED

#include <exy/scheduler/inline.hpp>
#include <exy/support/query.hpp>

namespace exy::queries
{
inline constexpr struct delegation_scheduler_t : exy::query_base
{
    static constexpr schedulers::inline_t default_value() noexcept
    {
        return {};
    }
} delegation_scheduler;
} // namespace exy::queries

#endif // EXY_QUERY_SCHEDULER_HPP_INCLUDED

