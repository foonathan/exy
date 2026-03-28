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
    struct _c
    {
        std::exception_ptr _ex   = {};
        std::atomic<bool>  _done = false;
        exy::storage<
            exy::max(F::storage_spec(), exy::storage_spec::get<std::optional<_value_type<F>>>())>
            _result;

        void _complete() noexcept
        {
            _done.store(true, std::memory_order_release);
            _done.notify_one();
        }

        static constexpr _c&                _get_self(exy::ctx_base& ctx) noexcept;
        static constexpr F&                 get_future(exy::ctx_base& ctx) noexcept;
        static constexpr exy::op_of<F, _c>& get_op(exy::ctx_base& ctx) noexcept;

        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            return exy::storage_ref(_get_self(ctx)._result);
        }

        static constexpr auto query(exy::query auto, auto&&...) noexcept
        {
            return exy::no_such_query{};
        }

        template <exy::signature_with_tag<exy::value_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = _get_self(ctx);
            auto  result = get_result_storage(ctx);

            auto [... args] = result.get<S>();
            result.emplace_raw<_value_type<F>>(std::in_place, exy_mov(args)...);

            self._complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = _get_self(ctx);
            auto  result = get_result_storage(ctx);

            auto [ex] = result.get<S>();
            self._ex  = ex;

            self._complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::stopped_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = _get_self(ctx);
            auto  result = get_result_storage(ctx);

            (void)result.get<S>();
            result.emplace_raw<_value_type<F>>(std::nullopt);

            self._complete();
            return nullptr;
        }
    };

    template <typename F>
    struct _ctx : exy::ctx_base, _c<F>
    {
        F&                   _f;
        exy::op_of<F, _c<F>> _op;

        constexpr explicit _ctx(F& f) noexcept : _f(f) {}
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::single_value_signatures<S> && exy::exception_error_signatures<S>
    static constexpr auto operator()(F&& f) noexcept(
        !_::mp_set_contains<S, exy::error_tag(std::exception_ptr)>::value
    )
    {
        _ctx<F> ctx(f);
        ctx._op.start(ctx);
        ctx._done.wait(false, std::memory_order_acquire);

        if (ctx._ex)
            std::rethrow_exception(ctx._ex);
        return exy::storage_ref(ctx._result).get_raw<_value_type<F>>();
    }
} sync_wait;

template <typename F>
constexpr sync_wait_t::_c<F>& sync_wait_t::_c<F>::_get_self(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx);
}

template <typename F>
constexpr F& sync_wait_t::_c<F>::get_future(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx)._f;
}

template <typename F>
constexpr exy::op_of<F, sync_wait_t::_c<F>>& sync_wait_t::_c<F>::get_op(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx)._op;
}
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

