// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_SCHEDULE_HPP_INCLUDED
#define EXY_FUTURE_SCHEDULE_HPP_INCLUDED

#include <exy/future/and_then.hpp>
#include <exy/support/adapter.hpp>
#include <exy/support/future.hpp>
#include <exy/support/scheduler.hpp>

namespace exy::futures
{
inline constexpr struct schedule_t
{
    template <exy::scheduler Scheduler>
    static constexpr exy::future auto operator()(const Scheduler& sch)
    {
        return sch.schedule();
    }
} schedule;

inline constexpr struct starts_on_t
{
    template <exy::scheduler Scheduler, exy::future F>
    static constexpr exy::future auto operator()(const Scheduler& sch, F&& f)
    {
        return futures::and_then(sch.schedule(), [f = exy_mov(f)]() mutable noexcept {
            return exy_mov(f);
        });
    }
} starts_on;

template <typename Derived, typename Signatures, typename SchF, typename Cont>
struct _co_impl_op
{
    using _base_values = _::mp_filter<exy::value_tag::is, Signatures>;

    struct _c
    {
        static constexpr SchF& get_future(exy::ctx_base& ctx) noexcept
        {
            return Derived::_get_scheduler_future(ctx);
        }
        static constexpr exy::op_of<SchF, _c>& get_op(exy::ctx_base& ctx) noexcept
        {
            _co_impl_op& self = Cont::get_op(ctx);
            return self._sch;
        }
        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            _co_impl_op& self = Cont::get_op(ctx);
            return exy::storage_ref(self._sch_result);
        }

        template <std::same_as<exy::value_tag()> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            _co_impl_op& self = Cont::get_op(ctx);

            exy::storage_ref sch_result = get_result_storage(ctx);
            (void)sch_result.template get<S>();

            // Continue with the correct result.
            if constexpr (_::mp_size<_base_values>::value == 0)
                exy_assert(false);
            else if constexpr (_::mp_size<_base_values>::value == 1)
                EXY_TAIL_CALL Cont::template call<_::mp_only<_base_values>>(ctx);
            else
                EXY_TAIL_CALL self._cont(ctx);
        }

        template <exy::signature_with_tag<exy::error_tag> S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            _co_impl_op& self = Cont::get_op(ctx);

            // Scheduling failed, destroy the previous result, and continue with the error.
            {
                exy::storage_ref result = Cont::get_result_storage(ctx);
                if constexpr (_::mp_size<_base_values>::value == 1)
                {
                    (void)result.template get<_::mp_only<_base_values>>();
                }
                else if constexpr (_::mp_size<_base_values>::value > 1)
                {
                    [&]<typename... PrevS>(exy::signatures<PrevS...>) {
                        auto matched
                            = ((self._cont == &Cont::template call<PrevS>
                                ? (void)result.template get<PrevS>(),
                                true : false)
                               || ...);
                        exy_assert(matched);
                    }(_base_values{});
                }

                exy::storage_ref sch_result = get_result_storage(ctx);
                auto [... args]             = sch_result.get<S>();
                result.template emplace<S>(exy_mov(args)...);
            }

            EXY_TAIL_CALL Cont::template call<S>(ctx);
        }
    };

    using sch_result_t = exy::storage<exy::storage_spec::get(exy::signatures_of<SchF>())>;
    using cont_t       = std::conditional_t<
        _::mp_size<_base_values>::value <= 1, exy::ctx_base, exy::continuation>;

    EXY_NO_UNIQUE_ADDRESS exy::op_of<SchF, _c> _sch;
    EXY_NO_UNIQUE_ADDRESS sch_result_t         _sch_result;
    EXY_NO_UNIQUE_ADDRESS cont_t               _cont;

    template <exy::signature_with_tag<exy::value_tag> S>
    static constexpr exy::continuation _schedule(exy::ctx_base& ctx) noexcept
    {
        _co_impl_op& self = Cont::get_op(ctx);
        if constexpr (_::mp_size<_base_values>::value > 1)
            self._cont = &Cont::template call<S>;

        return &self._sch.start;
    }
};

template <typename Base, typename SchF>
struct _co : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS SchF _sch;

    using signatures = _::mp_unique<_::mp_append<
        exy::signatures_of<Base>, _::mp_filter<exy::error_tag::is, exy::signatures_of<SchF>>>>;

    static consteval auto storage_spec() noexcept
    {
        // Note: we don't need the _sch_future::storage_spec; it lives in the state.
        return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
    }

    template <typename Cont>
    struct op : _co_impl_op<op<Cont>, exy::signatures_of<Base>, SchF, Cont>
    {
        static constexpr SchF& _get_scheduler_future(exy::ctx_base& ctx) noexcept
        {
            return Cont::get_future(ctx)._sch;
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
                return self.template _schedule<S>(ctx);
            }
        };
        EXY_NO_UNIQUE_ADDRESS exy::op_of<Base, _c> _base;

        static constexpr void* start(exy::ctx_base& ctx)
        {
            op&           self = Cont::get_op(ctx);
            EXY_TAIL_CALL self._base.start(ctx);
        }
    };
};

inline constexpr struct continues_on_t
{
    template <exy::future F, exy::scheduler Sch>
    static constexpr auto operator()(F&& f, const Sch& sch) -> _co<F, exy::future_for<Sch>>
    {
        return {{}, exy_mov(f), sch.schedule()};
    }

    template <exy::scheduler Sch>
    static constexpr auto operator()(const Sch& sch)
    {
        return exy::make_adaptor_proxy<continues_on_t>(sch);
    }
} continues_on;
} // namespace exy::futures

#endif // EXY_FUTURE_SCHEDULE_HPP_INCLUDED

