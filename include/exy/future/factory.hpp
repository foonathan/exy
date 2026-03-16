// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_FACTORY_HPP_INCLUDED
#define EXY_FUTURE_FACTORY_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy::futures
{
template <typename Tag, typename T>
struct _f : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS T _value;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<Tag(T)>, std::is_nothrow_move_constructible_v<T>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS T _value;

        constexpr explicit state(_f&& self) : _value(exy_mov(self)._value) {}
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
            EXY_TAIL_CALL
            exy::set_or_exception<Cont, Tag(T)>(result, [&] noexcept -> T&& {
                return exy_mov(self._value);
            })(s, result);
        }
    };
};

template <typename Tag>
struct factory_t
{
    template <exy::movable T>
    static constexpr _f<Tag, std::decay_t<T>> operator()(T&& value)
    {
        return {{}, exy_fwd(value)};
    }
};

inline constexpr factory_t<exy::value_tag> value;
inline constexpr factory_t<exy::error_tag> error;
} // namespace exy::futures

#endif // EXY_FUTURE_FACTORY_HPP_INCLUDED

