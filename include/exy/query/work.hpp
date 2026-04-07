// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_QUERY_WORK_HPP_INCLUDED
#define EXY_QUERY_WORK_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/query.hpp>

namespace exy::queries
{
inline constexpr struct inline_work_t : exy::query_base
{
    using result_type = exy::continuation;

    static constexpr exy::continuation default_value() noexcept
    {
        return nullptr;
    }
} inline_work;
} // namespace exy::queries

#endif // EXY_QUERY_WORK_HPP_INCLUDED

