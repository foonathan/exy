// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SUPPORT_INVOKE_HPP_INCLUDED
#define EXY_SUPPORT_INVOKE_HPP_INCLUDED

#include <exy/support/base.hpp>

#define exy_invoke(fn, ...) (fn)(__VA_ARGS__)

namespace exy
{
template <typename Fn, typename... Args>
concept invocable
    = requires (Fn&& fn, Args&&... args) { exy_invoke(exy_fwd(fn), exy_fwd(args)...); };
template <typename Fn, typename... Args>
using is_invocable = std::bool_constant<invocable<Fn, Args...>>;

template <typename Fn, typename... Args>
concept nothrow_invocable = invocable<Fn, Args...> && requires (Fn&& fn, Args&&... args) {
    { exy_invoke(exy_fwd(fn), exy_fwd(args)...) } noexcept;
};
template <typename Fn, typename... Args>
using is_nothrow_invocable = std::bool_constant<nothrow_invocable<Fn, Args...>>;

template <typename Fn, typename... Args>
using invoke_result_t = decltype(exy_invoke(std::declval<Fn>(), std::declval<Args>()...));
} // namespace exy

#endif // EXY_SUPPORT_INVOKE_HPP_INCLUDED

