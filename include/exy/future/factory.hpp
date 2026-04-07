// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_FACTORY_HPP_INCLUDED
#define EXY_FUTURE_FACTORY_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/invoke.hpp>

namespace exy::futures
{
template <typename S, typename Fn>
struct _f : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Fn _fn;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<S>, exy::is_nothrow_invocable<Fn, exy::storage_ref>::value>;

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::ctx_base& ctx)
        {
            _f&              self   = Cont::get_future(ctx);
            exy::storage_ref result = Cont::get_result_storage(ctx);

            auto cont = [&] {
                try
                {
                    exy_invoke(self._fn, result);
                    return &Cont::template call<S>;
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(result);
                }
            }();
            EXY_TAIL_CALL cont(ctx);
        }
    };
};

template <typename Tag>
struct factory_t
{
    template <typename... Ts>
    using _signature = Tag(std::decay_t<Ts>...);

    template <typename... Ts>
    static constexpr auto _factory(Ts&&... args)
    {
        constexpr auto nothrow = (std::is_nothrow_move_constructible_v<std::decay_t<Ts>> && ...);
        return [... args = exy_fwd(args)](exy::storage_ref result) mutable noexcept(nothrow) {
            result.emplace<_signature<Ts...>>(exy_mov(args)...);
        };
    }

    template <exy::movable... Ts>
    static constexpr auto operator()(Ts&&... args)
        -> _f<_signature<Ts...>, decltype(_factory(exy_fwd(args)...))>
    {
        return {{}, _factory(exy_fwd(args)...)};
    }
};

inline constexpr factory_t<exy::value_tag>   value;
inline constexpr factory_t<exy::error_tag>   error;
inline constexpr factory_t<exy::stopped_tag> stopped;

template <typename Tag>
struct run_t
{
    template <typename Fn>
    static constexpr auto _factory(Fn&& fn)
    {
        return [fn = exy_fwd(fn)](exy::storage_ref result) mutable noexcept(
                   noexcept(result.emplace_result(exy_mov(fn)))
               ) { //
            result.emplace_result(exy_mov(fn));
        };
    }

    template <exy::movable Fn>
        requires exy::invocable<Fn>
    static constexpr auto operator()(Fn&& fn)
        -> _f<Tag(exy::invoke_result_t<Fn>), decltype(_factory(exy_fwd(fn)))>
    {
        return {{}, _factory(exy_fwd(fn))};
    }
};

inline constexpr run_t<exy::value_tag> run;
} // namespace exy::futures

#endif // EXY_FUTURE_FACTORY_HPP_INCLUDED

