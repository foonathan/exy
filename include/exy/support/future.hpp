// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_FUTURE_HPP_INCLUDED
#define EXY_SUPPORT_FUTURE_HPP_INCLUDED

// IWYU pragma: begin_exports
#include <exy/support/base.hpp>
#include <exy/support/signature.hpp>
#include <exy/support/state.hpp>
#include <exy/support/storage.hpp>
// IWYU pragma: end_exports

namespace exy
{
struct future_base
{};

template <typename T>
concept future = std::derived_from<T, future_base>;

template <typename F>
using signatures_of = typename F::signatures;

using continuation = void* (*)(exy::state_ref, exy::storage_ref);

template <typename Cont, typename S>
constexpr auto set(exy::storage_ref result, auto&&... args) noexcept(
    std::is_nothrow_constructible_v<exy::signature_argument<S>, decltype(args)...>
) -> continuation
{
    result.emplace<exy::signature_argument<S>>(exy_fwd(args)...);
    return &Cont::template call<S>;
}

template <typename Cont>
constexpr auto set_exception(exy::storage_ref result) noexcept -> continuation
{
    return exy::set<Cont, exy::error_tag(std::exception_ptr)>(result, std::current_exception());
}

template <typename Cont, typename S>
constexpr auto set_or_exception(exy::storage_ref result, auto fn) noexcept -> continuation
{
    using type = exy::signature_argument<S>;
    if constexpr (std::is_nothrow_move_constructible_v<type> && noexcept(fn()))
    {
        return exy::set<Cont, S>(result, fn());
    }
    else
    {
        try
        {
            return exy::set<Cont, S>(result, fn());
        }
        catch (...)
        {
            return exy::set_exception<Cont>(result);
        }
    }
}
} // namespace exy

#endif // EXY_SUPPORT_FUTURE_HPP_INCLUDED

