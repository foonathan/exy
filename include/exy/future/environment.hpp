// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_ENVIRONMENT_HPP_INCLUDED
#define EXY_FUTURE_ENVIRONMENT_HPP_INCLUDED

#include <exy/support/adapter.hpp>
#include <exy/support/invoke.hpp>
#include <exy/support/query.hpp>

namespace exy::futures
{
template <typename Q>
struct _re : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Q _q;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<exy::value_tag(exy::typed_query_result_t<Q>)>,
        std::is_nothrow_move_constructible_v<exy::typed_query_result_t<Q>>>;

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::ctx_base& ctx)
        {
            exy::storage_ref result = Cont::get_result_storage(ctx);

            auto cont = [&] {
                try
                {
                    return exy::set<signatures, Cont, exy::value_tag(exy::typed_query_result_t<Q>)>(
                        result, exy::query_or_default<Cont>(Cont::get_future(ctx)._q, ctx)
                    );
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

template <typename T, typename Fn, bool NoexceptFn, typename... Q>
struct _ref : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS exy::pack<Q...> _q;
    EXY_NO_UNIQUE_ADDRESS Fn              _fn;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<exy::value_tag(T)>, std::is_nothrow_move_constructible_v<T> && NoexceptFn>;

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::ctx_base& ctx)
        {
            _ref&            self   = Cont::get_future(ctx);
            exy::storage_ref result = Cont::get_result_storage(ctx);

            auto cont = [&] {
                try
                {
                    auto [... q] = self._q;
                    return exy::set<signatures, Cont, exy::value_tag(T)>(
                        result, exy_invoke(self._fn, exy::query_or_default<Cont>(q, ctx)...)
                    );
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

constexpr auto read_env(exy::typed_query auto q) noexcept -> _re<decltype(q)>
{
    return {{}, q};
}

template <typename T, bool NoexceptFn = false, exy::movable Fn, exy::query... Q>
    requires (!exy::query<std::decay_t<Fn>>) && (sizeof...(Q) > 0)
constexpr auto read_env(Fn&& fn, Q... q) noexcept(std::is_nothrow_move_constructible_v<Fn>)
    -> _ref<T, std::decay_t<Fn>, NoexceptFn, Q...>
{
    return {{}, exy::make_pack(q...), exy_fwd(fn)};
}
} // namespace exy::futures

#endif // EXY_FUTURE_ENVIRONMENT_HPP_INCLUDED

