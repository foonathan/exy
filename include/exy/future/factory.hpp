// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_FACTORY_HPP_INCLUDED
#define EXY_FUTURE_FACTORY_HPP_INCLUDED

#include <exy/support/future.hpp>

namespace exy::futures
{
template <typename Tag, typename... Ts>
struct _f : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS exy::pack<Ts...> _pack;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<Tag(Ts...)>, (std::is_nothrow_move_constructible_v<Ts> && ...)>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::pack<Ts...> _pack;

        constexpr explicit state(_f&& self) noexcept(
            (std::is_nothrow_move_constructible_v<Ts> && ...)
        )
        : _pack(exy_mov(self)._pack)
        {}
    };

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::state_base& s, exy::storage_ref result)
        {
            state& self = Cont::get(s);

            auto cont = [&] {
                try
                {
                    result.emplace_raw<exy::pack<Ts...>>(exy_mov(self._pack));
                    return &Cont::template call<Tag(Ts...)>;
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(result);
                }
            }();
            EXY_TAIL_CALL cont(s, result);
        }
    };
};

template <typename Tag>
struct factory_t
{
    template <exy::movable... Ts>
    static constexpr _f<Tag, std::decay_t<Ts>...> operator()(Ts&&... args)
    {
        return {{}, exy::make_pack(exy_fwd(args)...)};
    }
};

inline constexpr factory_t<exy::value_tag>   value;
inline constexpr factory_t<exy::error_tag>   error;
inline constexpr factory_t<exy::stopped_tag> stopped;
} // namespace exy::futures

#endif // EXY_FUTURE_FACTORY_HPP_INCLUDED

