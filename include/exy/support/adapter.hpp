// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

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
struct adapter_continuation : private Cont
{
    using Cont::get_result_storage;
    using Cont::query;

    template <typename S>
    static constexpr void* call(exy::ctx_base& ctx)
    {
        if constexpr (requires { Derived::template continuation_for<S>(ctx); })
        {
            auto cont = Derived::template continuation_for<S>(ctx);
            if (cont)
                EXY_TAIL_CALL cont(ctx);
            else
                return nullptr;
        }
        else
        {
            return Cont::template call<S>(ctx);
        }
    }
};
} // namespace exy

#endif // EXY_SUPPORT_ADAPTER_HPP_INCLUDED

