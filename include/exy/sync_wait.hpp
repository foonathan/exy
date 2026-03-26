// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SYNC_WAIT_HPP_INCLUDED
#define EXY_SYNC_WAIT_HPP_INCLUDED

#include <atomic>
#include <optional>
#include <tuple>
#include <exy/support/future.hpp>
#include <exy/support/query.hpp>

namespace exy
{
inline constexpr struct sync_wait_t
{
    template <typename F>
    using _value_type = std::optional<exy::signatures_fold_tag<
        exy::signatures_of<F>, exy::value_tag, _::mp_quote<std::type_identity_t>,
        _::mp_quote<std::tuple>>>;

    template <typename F>
    struct _state : exy::state_base
    {
        F&                              _f;
        exy::state_of<F>                _s;
        std::exception_ptr              _ex   = {};
        std::atomic<bool>               _done = false;
        exy::storage<F::storage_spec()> _result;

        constexpr explicit _state(F& f) noexcept : _f(f) {}

        void complete() noexcept
        {
            _done.store(true, std::memory_order_release);
            _done.notify_one();
        }
    };

    template <typename F>
    struct _c
    {
        static constexpr F& get_future(exy::state_base& s) noexcept
        {
            return static_cast<_state<F>&>(s)._f;
        }
        static constexpr exy::state_of<F>& get_state(exy::state_base& s) noexcept
        {
            return static_cast<_state<F>&>(s)._s;
        }
        static constexpr exy::storage_ref get_result_storage(exy::state_base& s) noexcept
        {
            return exy::storage_ref(static_cast<_state<F>&>(s)._result);
        }

        static constexpr auto query(exy::query auto, exy::state_base&) noexcept
        {
            return exy::no_such_query{};
        }

        template <exy::signature_with_tag<exy::value_tag> S>
        static constexpr void* call(exy::state_base& s)
        {
            auto& self   = static_cast<_state<F>&>(s);
            auto  result = get_result_storage(s);

            auto [... args] = result.get<S>();
            result.emplace_raw<_value_type<F>>(std::in_place, exy_mov(args)...);

            self.complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::state_base& s)
        {
            auto& self   = static_cast<_state<F>&>(s);
            auto  result = get_result_storage(s);

            auto [ex] = result.get<S>();
            self._ex  = ex;

            self.complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::stopped_tag> S>
        static constexpr void* call(exy::state_base& s)
        {
            auto& self   = static_cast<_state<F>&>(s);
            auto  result = get_result_storage(s);

            (void)result.get<S>();
            result.emplace_raw<_value_type<F>>(std::nullopt);

            self.complete();
            return nullptr;
        }
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::single_value_signatures<S> && exy::exception_error_signatures<S>
    static constexpr auto operator()(F&& f) noexcept(
        !_::mp_set_contains<S, exy::error_tag(std::exception_ptr)>::value
    )
    {
        _state<F> state(f);

        F::template op<_c<F>>::start(state);
        state._done.wait(false, std::memory_order_acquire);

        if (state._ex)
            std::rethrow_exception(state._ex);
        return exy::storage_ref(state._result).get_raw<_value_type<F>>();
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

