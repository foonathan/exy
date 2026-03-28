// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SUPPORT_QUERY_HPP_INCLUDED
#define EXY_SUPPORT_QUERY_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
struct query_base
{};

template <typename T>
concept query = std::derived_from<T, query_base>;

template <typename T>
concept typed_query = query<T> && requires { typename T::result_type; };
template <typed_query T>
using typed_query_result_t = typename T::result_type;

struct no_such_query
{};

template <typename Env, typename Q, typename... Args>
using query_result_t = decltype(Env::query(std::declval<Q>(), std::declval<Args...>()));

template <typename Env, typename Q, typename... Args>
concept has_query = query<Q> && !std::same_as<query_result_t<Env, Q, Args...>, no_such_query>;

template <typename Q>
using query_default_value_t = decltype(std::declval<Q>().default_value());

template <typename Env, typename Q, typename... Args>
using query_or_default_result_t = typename std::conditional_t<
    has_query<Env, Q, Args...>, _::mp_defer<query_result_t, Env, Q, Args...>,
    _::mp_defer<query_default_value_t, Q>>::type;

template <typename Env>
constexpr auto query_or_default(query auto q, auto&&... args) noexcept
{
    if constexpr (has_query<Env, decltype(q), decltype(args)...>)
        return Env::query(q, exy_fwd(args)...);
    else if constexpr (requires { q.default_value(); })
        return q.default_value();
    else
        static_assert(_::mp_error<decltype(q)>::value, "query not supported");
}
} // namespace exy

#endif // EXY_SUPPORT_QUERY_HPP_INCLUDED

