// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_YIELD_HPP_INCLUDED
#define EXY_FUTURE_YIELD_HPP_INCLUDED

#include <optional>
#include <exy/future/schedule.hpp>
#include <exy/query/scheduler.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Base>
struct _y : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_of<Base>, /* we cannot know whether the scheduler fails*/ false>;

    static consteval auto storage_spec() noexcept
    {
        return Base::storage_spec();
    }

    template <typename Cont>
    using _scheduler
        = exy::query_or_default_result_t<Cont, queries::delegation_scheduler_t, exy::ctx_base&>;
    template <typename Cont>
    using _scheduler_future = exy::future_for<_scheduler<Cont>>;

    template <typename Cont>
    struct op : _co_impl_op<op<Cont>, exy::signatures_of<Base>, _scheduler_future<Cont>, Cont>
    {
        static constexpr _scheduler_future<Cont>& _get_scheduler_future(exy::ctx_base& ctx) noexcept
        {
            return *Cont::get_op(ctx)._sch;
        }

        struct _c : exy::adapter_continuation<_c, Cont>
        {
            static constexpr Base& get_future(exy::ctx_base& ctx) noexcept
            {
                return Cont::get_future(ctx)._base;
            }
            static constexpr exy::op_of<Base, _c>& get_op(exy::ctx_base& ctx) noexcept
            {
                return Cont::get_op(ctx)._base;
            }

            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(exy::ctx_base& ctx) noexcept
            {
                op& self = Cont::get_op(ctx);

                try
                {
                    self._sch.emplace(
                        exy::query_or_default<Cont>(queries::delegation_scheduler, ctx).schedule()
                    );
                    return self.template _schedule<S>(ctx);
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(Cont::get_result_storage(ctx));
                }
            }
        };

        EXY_NO_UNIQUE_ADDRESS exy::op_of<Base, _c> _base;
        std::optional<_scheduler_future<Cont>>     _sch;

        static constexpr void* start(exy::ctx_base& ctx)
        {
            op&           self = Cont::get_op(ctx);
            EXY_TAIL_CALL self._base.start(ctx);
        }
    };
};

inline constexpr struct yield_t : exy::adapter
{
    template <exy::future Base>
    static constexpr auto operator()(Base&& base) -> _y<Base>
    {
        return {{}, exy_mov(base)};
    }
} yield;
} // namespace exy::futures

#endif // EXY_FUTURE_YIELD_HPP_INCLUDED

