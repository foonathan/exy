// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_SCHEDULE_HPP_INCLUDED
#define EXY_FUTURE_SCHEDULE_HPP_INCLUDED

#include <variant>
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

template <typename Base, typename SchF>
struct _co : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS SchF _sch;

    using signatures = _::mp_unique<_::mp_append<
        exy::signatures_of<Base>, _::mp_filter<exy::error_tag::is, exy::signatures_of<SchF>>>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::state_of<Base> _base;
        EXY_NO_UNIQUE_ADDRESS exy::state_of<SchF>                        _sch;
        exy::storage_ref                                                 _prev_result;
        exy::storage<exy::storage_spec::get(exy::signatures_of<SchF>())> _sch_result;

        constexpr explicit state(_co& self) noexcept(
            std::is_nothrow_constructible_v<exy::state_of<Base>, Base&>
            && std::is_nothrow_constructible_v<exy::state_of<SchF>, SchF&>
        )
        : _base(self._base), _sch(self._sch)
        {}
    };

    static consteval auto storage_spec() noexcept
    {
        // Note: we don't need the _sch_future::storage_spec; it lives in the state.
        return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
    }

    template <typename Cont>
    struct op
    {
        template <typename PrevS>
        struct _cpost
        {
            static constexpr SchF& get_future(exy::state_base& s) noexcept
            {
                return Cont::get_future(s)._sch;
            }
            static constexpr exy::state_of<SchF>& get_state(exy::state_base& s) noexcept
            {
                return Cont::get_state(s)._sch;
            }

            template <std::same_as<exy::value_tag()> S>
            static constexpr void* call(exy::state_base& s, exy::storage_ref result)
            {
                state& self = Cont::get_state(s);

                // Continue with the correct result.
                result = self._prev_result;
                EXY_TAIL_CALL Cont::template call<PrevS>(s, result);
            }

            template <exy::signature_with_tag<exy::error_tag> S>
            static constexpr void* call(exy::state_base& s, exy::storage_ref result)
            {
                state& self = Cont::get_state(s);

                // Scheduling failed, destroy the previous result, and continue with the error.
                {
                    (void)self._prev_result.template get<PrevS>();

                    auto [... args] = result.get<S>();
                    self._prev_result.template emplace<S>(exy_mov(args)...);
                }

                result = self._prev_result;
                EXY_TAIL_CALL Cont::template call<S>(s, result);
            }
        };

        struct _cpre : exy::adapter_continuation<_cpre, Cont>
        {
            static constexpr Base& get_future(exy::state_base& s) noexcept
            {
                return Cont::get_future(s)._base;
            }
            static constexpr exy::state_of<Base>& get_state(exy::state_base& s) noexcept
            {
                return Cont::get_state(s)._base;
            }

            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(
                exy::state_base& s, exy::storage_ref& result
            ) noexcept
            {
                state& self = Cont::get_state(s);

                // Redirect the storage for the schedule operation.
                self._prev_result = result;
                result            = exy::storage_ref(self._sch_result);

                // And continue with performing the schedule.
                return &SchF::template op<_cpost<S>>::start;
            }
        };

        static constexpr void* start(exy::state_base& s, exy::storage_ref result)
        {
            EXY_TAIL_CALL Base::template op<_cpre>::start(s, result);
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

