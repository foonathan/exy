// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_DETACH_HPP_INCLUDED
#define EXY_DETACH_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/query.hpp>

namespace exy
{
inline constexpr struct detach_t
{
    template <typename F>
    struct _state : exy::state_base
    {
        F                               _f;
        exy::state_of<F>                _s;
        exy::storage<F::storage_spec()> _result;

        constexpr explicit _state(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>)
        : _f(exy_mov(f))
        {}
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

        template <typename S>
        static constexpr void* call(exy::state_base& s)
        {
            auto self = static_cast<_state<F>*>(&s);
            (void)exy::storage_ref(self->_result).get<S>(); // destroy
            if constexpr (exy::signature_with_tag<S, exy::error_tag>)
                std::terminate(); // unhandled exception

            delete self;
            return nullptr;
        }
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::void_value_signature<S>
    static constexpr void operator()(F&& f)
    {
        auto state = new _state(exy_mov(f));
        F::template op<_c<F>>::start(*state);
    }
} detach;
} // namespace exy

#endif // EXY_DETACH_HPP_INCLUDED

