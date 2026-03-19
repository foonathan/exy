// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

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

    using state = exy::state_base;

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::state_base& s, exy::storage_ref result)
        {
            auto cont = [&] {
                try
                {
                    return exy::set<signatures, Cont, exy::value_tag(exy::typed_query_result_t<Q>)>(
                        result, exy::query_or_default<Cont>(Cont::get_future(s)._q, s)
                    );
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

inline constexpr struct read_env_t
{
    static constexpr auto operator()(exy::typed_query auto q) noexcept -> _re<decltype(q)>
    {
        return {{}, q};
    }
} read_env;

template <typename T, typename Fn, bool NoexceptFn, typename... Q>
struct _we : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS exy::pack<Q...> _q;
    EXY_NO_UNIQUE_ADDRESS Fn              _fn;

    using signatures = exy::signatures_insert_exception<
        exy::signatures<exy::value_tag(T)>, std::is_nothrow_move_constructible_v<T> && NoexceptFn>;

    using state = exy::state_base;

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        static constexpr void* start(exy::state_base& s, exy::storage_ref result)
        {
            _we& self = Cont::get_future(s);
            auto cont = [&] {
                try
                {
                    auto [... q] = self._q;
                    return exy::set<signatures, Cont, exy::value_tag(T)>(
                        result, exy_invoke(self._fn, exy::query_or_default<Cont>(q, s)...)
                    );
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

template <typename T, bool NoexceptFn = false>
struct with_env_t
{
    template <typename Fn, exy::query... Q>
    static constexpr auto operator()(Fn&& fn, Q... q) noexcept(
        std::is_nothrow_move_constructible_v<Fn>
    ) -> _we<T, std::decay_t<Fn>, NoexceptFn, Q...>
    {
        return {{}, exy::make_pack(q...), exy_fwd(fn)};
    }
};

template <typename T, bool NoexceptFn = false>
inline constexpr with_env_t<T, NoexceptFn> with_env;
} // namespace exy::futures

#endif // EXY_FUTURE_ENVIRONMENT_HPP_INCLUDED

