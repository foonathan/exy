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

template <typename Base, typename Sch>
struct _co : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Sch  _sch;

    using _sch_future       = exy::future_for<Sch>;
    using _sch_future_state = exy::state_of<_sch_future>;

    using signatures = _::mp_unique<_::mp_append<
        exy::signatures_of<Base>,
        _::mp_filter<exy::error_tag::is, exy::signatures_of<_sch_future>>>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::state_of<Base>                               _base;
        std::variant<Sch, _sch_future_state>                                    _sch_or_sch_state;
        exy::storage_ref                                                        _prev_result;
        exy::storage<exy::storage_spec::get(exy::signatures_of<_sch_future>())> _sch_result;

        constexpr explicit state(_co&& self) noexcept(
            std::is_nothrow_constructible_v<exy::state_of<Base>, Base&&>
            && std::is_nothrow_move_constructible_v<Sch>
        )
        : _base(exy_mov(self)._base), _sch_or_sch_state(exy_mov(self)._sch)
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
            static constexpr _sch_future_state& get(exy::state_base& s) noexcept
            {
                return std::get<_sch_future_state>(Cont::get(s)._sch_or_sch_state);
            }

            template <std::same_as<exy::value_tag()> S>
            static constexpr void* call(exy::state_base& s, exy::storage_ref result)
            {
                state& self = Cont::get(s);

                // Continue with the correct result.
                result = self._prev_result;
                EXY_TAIL_CALL Cont::template call<PrevS>(s, result);
            }

            template <exy::signature_with_tag<exy::error_tag> S>
            static constexpr void* call(exy::state_base& s, exy::storage_ref result)
            {
                state& self = Cont::get(s);

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
            static constexpr exy::state_of<Base>& get(exy::state_base& s) noexcept
            {
                return Cont::get(s)._base;
            }

            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(
                exy::state_base& s, exy::storage_ref& result
            ) noexcept
            {
                state& self = Cont::get(s);

                // Prepare the state for the schedule operation.
                auto sch = std::get<Sch>(exy_mov(self)._sch_or_sch_state);
                self._sch_or_sch_state.template emplace<_sch_future_state>(exy_mov(sch).schedule());

                // Redirect the storage for the schedule operation.
                self._prev_result = result;
                result            = exy::storage_ref(self._sch_result);

                // And continue with performing the schedule.
                static constexpr auto sch_state_path
                    = [](exy::state_base* s) -> _sch_future_state& {
                    return std::get<_sch_future_state>(static_cast<state*>(s)->_sch_or_sch_state);
                };
                return &_sch_future::template op<_cpost<S>>::start;
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
    static constexpr auto operator()(F&& f, const Sch& sch) -> _co<F, Sch>
    {
        return {{}, exy_mov(f), sch};
    }

    template <exy::scheduler Sch>
    static constexpr auto operator()(const Sch& sch)
    {
        return exy::make_adaptor_proxy<continues_on_t>(sch);
    }
} continues_on;
} // namespace exy::futures

#endif // EXY_FUTURE_SCHEDULE_HPP_INCLUDED

