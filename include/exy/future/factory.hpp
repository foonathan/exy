// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_FACTORY_HPP_INCLUDED
#define EXY_FUTURE_FACTORY_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy::futures
{
template <typename T>
struct _v : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS T _value;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<exy::value_tag(T)>, std::is_nothrow_move_constructible_v<T>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS T _value;

        constexpr explicit state(_v&& self) : _value(exy_mov(self)._value) {}
    };

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont, auto... Path>
    struct op
    {
        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            state& self = s.get<Path...>();

            auto cont = [&] noexcept {
                if constexpr (std::is_nothrow_move_constructible_v<T>)
                {
                    result.emplace<T>(exy_mov(self._value));
                    return &Cont::template call<exy::value_tag(T)>;
                }
                else
                {
                    try
                    {
                        result.emplace<T>(exy_mov(self._value));
                        return &Cont::template call<exy::value_tag(T)>;
                    }
                    catch (...)
                    {
                        result.emplace<std::exception_ptr>(std::current_exception());
                        return &Cont::template call<exy::error_tag(std::exception_ptr)>;
                    }
                }
            }();
            EXY_TAIL_CALL cont(s, result);
        }
    };
};

inline constexpr struct value_t
{
    template <exy::movable T>
    static constexpr _v<std::decay_t<T>> operator()(T&& value)
    {
        return {{}, exy_fwd(value)};
    }
} value;
} // namespace exy::futures

#endif // EXY_FUTURE_FACTORY_HPP_INCLUDED

