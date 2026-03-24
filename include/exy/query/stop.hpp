// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_QUERY_STOP_HPP_INCLUDED
#define EXY_QUERY_STOP_HPP_INCLUDED

#include <exy/support/query.hpp>

namespace exy::queries
{
inline constexpr struct stop_requested_t : exy::query_base
{
    using result_type = bool;

    static constexpr bool default_value() noexcept
    {
        return false;
    }
} stop_requested;
} // namespace exy::queries

#endif // EXY_QUERY_STOP_HPP_INCLUDED

