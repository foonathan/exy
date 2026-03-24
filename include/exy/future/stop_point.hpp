// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_STOP_POINT_HPP_INCLUDED
#define EXY_FUTURE_STOP_POINT_HPP_INCLUDED

#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Base>
struct _sp : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;

    using signatures = _::mp_set_push_back<exy::signatures_of<Base>, exy::stopped_tag()>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::state_of<Base> _base;

        constexpr explicit state(_sp& self) noexcept(exy::has_nothrow_constructible_state<Base>)
        : _base(self._base)
        {}
    };

    static consteval auto storage_spec() noexcept
    {
        return Base::storage_spec();
    }

    template <typename Cont>
    struct op
    {
        struct _c : exy::adapter_continuation<_c, Cont>
        {
            static constexpr Base& get_future(exy::state_base& s) noexcept
            {
                return Cont::get_future(s)._base;
            }
            static constexpr exy::state_of<Base>& get_state(exy::state_base& s) noexcept
            {
                return Cont::get_state(s)._base;
            }

            template <typename S>
                requires exy::has_query<Cont, exy::queries::stop_requested_t, exy::state_base&>
                      && (!exy::signature_with_tag<S, exy::stopped_tag>)
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                if (!Cont::query(exy::queries::stop_requested, s))
                    return &Cont::template call<S>;

                exy::storage_ref result = Cont::get_result_storage(s);
                (void)result.get<S>(); // destroy
                return exy::set<signatures, Cont, exy::stopped_tag()>(result);
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            EXY_TAIL_CALL Base::template op<_c>::start(s);
        }
    };
};

inline constexpr struct stop_point_t : exy::adapter
{
    template <exy::future Base>
    static constexpr auto operator()(Base&& base) -> _sp<Base>
    {
        return {{}, exy_mov(base)};
    }
} stop_point;
} // namespace exy::futures

#endif // EXY_FUTURE_STOP_POINT_HPP_INCLUDED

