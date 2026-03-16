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
        exy_assert(false);
        return nullptr;
    }
}
} // namespace exy

#endif // EXY_SUPPORT_FUTURE_HPP_INCLUDED

