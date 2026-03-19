// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_FUTURE_HPP_INCLUDED
#define EXY_SUPPORT_FUTURE_HPP_INCLUDED

// IWYU pragma: begin_exports
#include <exy/support/base.hpp>
#include <exy/support/signature.hpp>
#include <exy/support/storage.hpp>
// IWYU pragma: end_exports

namespace exy
{
struct future_base
{};

template <typename T>
concept future = std::derived_from<T, future_base>;
template <typename T>
using is_future = std::bool_constant<future<T>>;

template <typename F>
using signatures_of = typename F::signatures;
template <typename F>
using state_of = typename F::state;
template <typename F>
concept has_nothrow_constructible_state = std::is_nothrow_constructible_v<state_of<F>, F&>;

struct state_base
{
    state_base() = default;

    template <exy::future F>
        requires std::same_as<state_of<F>, state_base>
    constexpr explicit state_base(F&) noexcept
    {}

    state_base(const state_base&)            = delete;
    state_base& operator=(const state_base&) = delete;
    ~state_base()                            = default;
};

using continuation = void* (*)(exy::state_base&, exy::storage_ref);

template <typename Signatures, typename Cont, typename S>
constexpr auto set(exy::storage_ref result, auto&&... args) -> continuation
{
    static_assert(_::mp_set_contains<Signatures, S>::value);
    result.emplace<S>(exy_fwd(args)...);
    return &Cont::template call<S>;
}

template <typename Signatures, typename Cont>
constexpr auto set_exception(exy::storage_ref result) noexcept -> continuation
{
    if constexpr (_::mp_set_contains<Signatures, exy::error_tag(std::exception_ptr)>::value)
    {
        return exy::set<Signatures, Cont, exy::error_tag(std::exception_ptr)>(
            result, std::current_exception()
        );
    }
    else
    {
        std::terminate();
    }
}
} // namespace exy

#endif // EXY_SUPPORT_FUTURE_HPP_INCLUDED

