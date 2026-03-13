// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SYNC_WAIT_HPP_INCLUDED
#define EXY_SYNC_WAIT_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
inline constexpr struct
{
    template <typename F>
    struct _state : exy::state_base
    {
        typename F::state _s;

        constexpr explicit _state(F&& f) : _s(exy_mov(f)) {}
    };

    struct _c
    {
        template <typename T>
        static constexpr auto resume(exy::state_ref, exy::storage_ref) noexcept
            -> std::coroutine_handle<>
        {
            return {};
        }
    };

    template <exy::future F>
    static constexpr auto operator()(F&& f)
    {
        _state<F>                       state(exy_mov(f));
        exy::storage<F::storage_spec()> result;
        F::template op<_c, &_state<F>::_s>::start(state, result);

        return exy::storage_ref(result).get<exy::signature_common_value_type<F::signature()>>();
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

