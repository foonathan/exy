// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SYNC_WAIT_HPP_INCLUDED
#define EXY_SYNC_WAIT_HPP_INCLUDED

#include <optional>
#include <tuple>
#include <exy/support/future.hpp>

namespace exy
{
template <typename S>
concept single_value_signatures
    = _::mp_size<exy::signatures_fold_tag<
          S, exy::value_tag, _::mp_quote<_::mp_list>, _::mp_quote<std::tuple>>>::value
   == 1;

template <typename S>
concept exception_error_signatures = exy::signatures_all_of_tag<
    S, exy::error_tag,
    _::mp_compose<
        _::mp_list, _::mp_bind_back<std::is_same, _::mp_list<std::exception_ptr>>::template fn>>;

inline constexpr struct sync_wait_t
{
    template <typename F>
    using _value_type = std::optional<exy::signatures_fold_tag<
        exy::signatures_of<F>, exy::value_tag, _::mp_quote<std::type_identity_t>,
        _::mp_quote<std::tuple>>>;

    template <typename F>
    struct _state : exy::state_base
    {
        exy::state_of<F> _s;

        constexpr explicit _state(
            F&& f
        ) noexcept(std::is_nothrow_constructible_v<exy::state_of<F>, F&&>)
        : _s(exy_mov(f))
        {}
    };

    template <typename T>
    struct _c
    {
        template <exy::signature_with_tag<exy::value_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            result.get<S>([&](auto&&... args) {
                result.emplace_raw<T>(std::in_place, exy_fwd(args)...);
            });
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            result.get<S>([&](const std::exception_ptr& e) { std::rethrow_exception(e); });
        }

        template <exy::signature_with_tag<exy::stopped_tag> S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result) noexcept
        {
            result.get<S>([](auto&&...) {});
            result.emplace_raw<T>(std::nullopt);
            return nullptr;
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
        return exy::storage_ref(result).get_raw<_value_type<F>>();
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

