// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SYNC_WAIT_HPP_INCLUDED
#define EXY_SYNC_WAIT_HPP_INCLUDED

#include <exy/support/future.hpp>

namespace exy
{
template <typename S>
concept single_value_signatures
    = exy::signatures_all_of_tag<S, exy::value_tag, _::mp_compose<_::mp_list, _::mp_is_unit_list>>;

template <typename S>
concept exception_error_signatures = exy::signatures_all_of_tag<
    S, exy::error_tag,
    _::mp_compose<
        _::mp_list, _::mp_bind_back<std::is_same, _::mp_list<std::exception_ptr>>::template fn>>;

inline constexpr struct
{
    template <typename F>
    using _value_type = exy::signatures_fold_tag<
        exy::signatures_of<F>, exy::value_tag, _::mp_quote<std::common_type_t>,
        _::mp_compose<_::mp_list, _::mp_only>>;

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
            result.get<S>([&](auto&&... args) {
                result.emplace<exy::value_tag(T)>(exy_fwd(args)...);
            });
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            result.get<S>([&](const std::exception_ptr& e) { std::rethrow_exception(e); });
        }
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::single_value_signatures<S> && exy::exception_error_signatures<S>
    static constexpr auto operator()(F&& f) noexcept(
        !_::mp_set_contains<S, exy::error_tag(std::exception_ptr)>::value
    )
    {
        _state<F>                       state(exy_mov(f));
        exy::storage<F::storage_spec()> result;
        F::template op<_c<_value_type<F>>, &_state<F>::_s>::start(state, result);
        return exy::storage_ref(result).get<exy::value_tag(_value_type<F>)>([&](auto&& arg) {
            return exy_fwd(arg);
        });
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

