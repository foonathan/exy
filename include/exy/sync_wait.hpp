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
    struct _c : exy::ctx_base
    {
        F&                              _f;
        exy::op_of<F, _c>               _op;
        std::exception_ptr              _ex   = {};
        std::atomic<bool>               _done = false;
        exy::storage<F::storage_spec()> _result;

        constexpr explicit _c(F& f) noexcept : _f(f) {}

        void _complete() noexcept
        {
            _done.store(true, std::memory_order_release);
            _done.notify_one();
        }

        static constexpr F& get_future(exy::ctx_base& ctx) noexcept
        {
            return static_cast<_c&>(ctx)._f;
        }
        static constexpr exy::op_of<F, _c>& get_op(exy::ctx_base& ctx) noexcept
        {
            return static_cast<_c&>(ctx)._op;
        }
        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            return exy::storage_ref(static_cast<_c&>(ctx)._result);
        }

        static constexpr auto query(exy::query auto, exy::ctx_base&) noexcept
        {
            return exy::no_such_query{};
        }

        template <exy::signature_with_tag<exy::value_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = static_cast<_c&>(ctx);
            auto  result = get_result_storage(ctx);

            auto [... args] = result.get<S>();
            result.emplace_raw<_value_type<F>>(std::in_place, exy_mov(args)...);

            self._complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = static_cast<_c&>(ctx);
            auto  result = get_result_storage(ctx);

            auto [ex] = result.get<S>();
            self._ex  = ex;

            self._complete();
            return nullptr;
        }

        template <exy::signature_with_tag<exy::stopped_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto& self   = static_cast<_c&>(ctx);
            auto  result = get_result_storage(ctx);

            (void)result.get<S>();
            result.emplace_raw<_value_type<F>>(std::nullopt);

            self._complete();
            return nullptr;
        }
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::single_value_signatures<S> && exy::exception_error_signatures<S>
    static constexpr auto operator()(F&& f) noexcept(
        !_::mp_set_contains<S, exy::error_tag(std::exception_ptr)>::value
    )
    {
        _c<F> ctx(f);
        ctx._op.start(ctx);
        ctx._done.wait(false, std::memory_order_acquire);

        if (ctx._ex)
            std::rethrow_exception(ctx._ex);
        return exy::storage_ref(ctx._result).get_raw<_value_type<F>>();
    }
} sync_wait;
} // namespace exy

#endif // EXY_SYNC_WAIT_HPP_INCLUDED

