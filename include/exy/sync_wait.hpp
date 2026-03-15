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
    using _value_type = exy::signatures_fold_tag<
        exy::signatures_of<F>, exy::value_tag, _::mp_quote<std::common_type_t>,
        _::mp_quote<exy::signature_argument>>;

    template <typename F>
    struct _state : exy::state_base
    {
        typename F::state _s;

        constexpr explicit _state(F&& f) : _s(exy_mov(f)) {}
    };

    template <typename T>
    struct _c
    {
        template <exy::signature_with_tag<exy::value_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            result.emplace<T>(result.template get<exy::signature_argument<S>>());
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            throw result.template get<exy::signature_argument<S>>();
        }
    };

    template <exy::future F>
    static constexpr auto operator()(F&& f)
    {
        _state<F>                       state(exy_mov(f));
        exy::storage<F::storage_spec()> result;
        F::template op<_c<_value_type<F>>, &_state<F>::_s>::start(state, result);
        return exy::storage_ref(result).get<_value_type<F>>();
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

