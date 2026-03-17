// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_SCHEDULER_HPP_INCLUDED
#define EXY_SUPPORT_SCHEDULER_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
struct scheduler_base
{};

template <typename T>
concept scheduler = std::derived_from<T, scheduler_base>;

template <scheduler Sch>
using future_for = decltype(std::declval<Sch>().schedule());
} // namespace exy

#endif // EXY_SUPPORT_SCHEDULER_HPP_INCLUDED

