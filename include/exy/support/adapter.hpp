// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_ADAPTER_HPP_INCLUDED
#define EXY_SUPPORT_ADAPTER_HPP_INCLUDED

#include <exy/support/future.hpp> // IWYU pragma: export
#include <exy/support/query.hpp>  // IWYU pragma: export

namespace exy
{
struct adapter
{
    template <exy::future F, typename Self>
        requires std::derived_from<std::decay_t<Self>, adapter>
    friend constexpr auto operator|(F&& lhs, Self&& self)
    {
        return exy_fwd(self)(exy_mov(lhs));
    }
};

template <typename Fn>
struct adapter_proxy : Fn, adapter
{
    explicit adapter_proxy(Fn&& fn) : Fn(exy_fwd(fn)) {}
};

template <typename Tag>
constexpr auto make_adaptor_proxy(exy::movable auto&&... args)
{
    return adapter_proxy([... args = exy_fwd(args)](auto&& head) mutable {
        return Tag()(exy_fwd(head), exy_fwd(args)...);
    });
}
} // namespace exy

namespace exy
{
template <typename Derived, typename Cont>
struct adapter_continuation
{
    static constexpr auto query(exy::query auto q, exy::state_base& s)
    {
        if constexpr (requires { Derived::override_query(q, s); })
            EXY_TAIL_CALL Derived::override_query(q, s);
        else
            EXY_TAIL_CALL Cont::query(q, s);
    }

    template <typename S>
    static constexpr void* call(exy::state_base& s, exy::storage_ref result)
    {
        if constexpr (requires { Derived::template continuation_for<S>(s, result); })
        {
            auto cont = Derived::template continuation_for<S>(s, result);
            if (cont)
                EXY_TAIL_CALL cont(s, result);
            else
                return nullptr;
        }
        else
        {
            return Cont::template call<S>(s, result);
        }
    }
};
} // namespace exy

#endif // EXY_SUPPORT_ADAPTER_HPP_INCLUDED

