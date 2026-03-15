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

    static consteval auto signature() noexcept
    {
        return exy::signature_t<exy::signature_value_t(T)>{};
    }

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS T _value;

        constexpr explicit state(_v&& self) : _value(exy_mov(self)._value) {}
    };

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signature());
    }

    template <typename Cont, auto... Path>
    struct op
    {
        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            state& self = s.get<Path...>();

            result.emplace<T>(exy_mov(self._value));
            EXY_TAIL_CALL Cont::template resume<T>(s, result);
        }
    };
};

inline constexpr struct value_t
{
    template <exy::movable T>
    static constexpr auto operator()(T&& value) EXY_RETURN(_v<std::decay_t<T>>{{}, exy_fwd(value)})
} value;
} // namespace exy::futures

#endif // EXY_FUTURE_FACTORY_HPP_INCLUDED

