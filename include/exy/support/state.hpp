// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_STATE_HPP_INCLUDED
#define EXY_SUPPORT_STATE_HPP_INCLUDED

#include <exy/support/base.hpp>
namespace exy
{
struct state_base
{};

class state_ref
{
public:
    constexpr state_ref(state_base& state) noexcept : _ptr(&state) {}

    template <auto... MemPtrs>
    constexpr auto& get() const noexcept
    {
        return []<auto Head, auto... Tail>(
                   this auto recurse, state_base* cur, exy::constant<Head> head,
                   exy::constant<Tail>... tail
               ) noexcept -> auto& {
            auto& obj = apply_member(cur, head);
            if constexpr (sizeof...(Tail) == 0)
                return obj;
            else
                return recurse(&obj, tail...);
        }(_ptr, exy::constant<MemPtrs>{}...);
    }

private:
    template <typename T, typename C, T C::* Mem>
    static constexpr T& apply_member(state_base* base, exy::constant<Mem>) noexcept
    {
        return static_cast<C*>(base)->*Mem;
    }

    template <auto Fn>
    static constexpr auto& apply_member(state_base* base, exy::constant<Fn>) noexcept
    {
        return Fn(base);
    }

    state_base* _ptr;
};
} // namespace exy

#endif // EXY_SUPPORT_STATE_HPP_INCLUDED

