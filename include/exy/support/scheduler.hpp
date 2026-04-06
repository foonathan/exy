// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SUPPORT_SCHEDULER_HPP_INCLUDED
#define EXY_SUPPORT_SCHEDULER_HPP_INCLUDED

#include <exy/support/base.hpp>
#include <exy/support/future.hpp>

namespace exy
{
struct scheduler_base
{};

template <typename T>
concept scheduler = std::derived_from<T, scheduler_base>;

template <scheduler Sch>
using future_for = decltype(std::declval<Sch>().schedule());

template <typename T>
concept nothrow_scheduler
    = scheduler<T> && noexcept(std::declval<T>().schedule())
   && exy::signatures_none_of_tag<exy::signatures_of<future_for<T>>, exy::error_tag>;
} // namespace exy

#endif // EXY_SUPPORT_SCHEDULER_HPP_INCLUDED

