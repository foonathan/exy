// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_REPEAT_HPP_INCLUDED
#define EXY_FUTURE_REPEAT_HPP_INCLUDED

#include <optional>
#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Base>
struct _r : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_replace_tag<
            exy::signatures_of<Base>, exy::value_tag, exy::signatures<exy::stopped_tag()>>,
        std::is_nothrow_copy_constructible_v<Base>>;

    struct _simple_state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS Base _future;
        EXY_NO_UNIQUE_ADDRESS exy::state_of<Base> _state;
    };

    struct _optional_state : exy::state_base
    {
        struct impl
        {
            EXY_NO_UNIQUE_ADDRESS Base future;
            EXY_NO_UNIQUE_ADDRESS exy::state_of<Base> state;

            explicit impl(const Base& base) : future(base) {}
        };
        std::optional<impl> _impl;
    };

    using state = std::conditional_t<
        std::is_default_constructible_v<Base> && std::is_copy_assignable_v<Base>
            && std::is_move_assignable_v<exy::state_of<Base>>,
        _simple_state, _optional_state>;

    static consteval auto storage_spec() noexcept
    {
        return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
    }

    template <typename Cont>
    struct op
    {
        static constexpr exy::continuation _restart(exy::state_base& s) noexcept
        {
            _r&    f    = Cont::get_future(s);
            state& self = Cont::get_state(s);
            try
            {
                if constexpr (std::same_as<state, _simple_state>)
                {
                    self._future = f._base;
                    self._state  = {};
                }
                else
                {
                    self._impl.emplace(f._base);
                }
                return &Base::template op<_c>::start;
            }
            catch (...)
            {
                return exy::set_exception<signatures, Cont>(Cont::get_result_storage(s));
            }
        }

        struct _c : exy::adapter_continuation<_c, Cont>
        {
            static constexpr Base& get_future(exy::state_base& s) noexcept
            {
                if constexpr (std::same_as<state, _simple_state>)
                    return Cont::get_state(s)._future;
                else
                    return Cont::get_state(s)._impl->future;
            }
            static constexpr exy::state_of<Base>& get_state(exy::state_base& s) noexcept
            {
                if constexpr (std::same_as<state, _simple_state>)
                    return Cont::get_state(s)._state;
                else
                    return Cont::get_state(s)._impl->state;
            }

            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                exy::storage_ref result = Cont::get_result_storage(s);
                (void)result.get<S>(); // destroy

                if (exy::query_or_default<Cont>(queries::stop_requested, s))
                    return exy::set<signatures, Cont, exy::stopped_tag()>(result);
                else
                    return _restart(s);
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            EXY_TAIL_CALL _restart(s)(s);
        }
    };
};

inline constexpr struct repeat_t : exy::adapter
{
    template <exy::future F, typename S = exy::signatures_of<F>>
        requires std::is_copy_constructible_v<F> && exy::void_value_signature<S>
    static constexpr _r<F> operator()(F&& f)
    {
        return {{}, exy_mov(f)};
    }
} repeat;
} // namespace exy::futures

#endif // EXY_FUTURE_REPEAT_HPP_INCLUDED

