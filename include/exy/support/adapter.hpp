// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_ADAPTER_HPP_INCLUDED
#define EXY_SUPPORT_ADAPTER_HPP_INCLUDED

#include <exy/support/future.hpp> // IWYU pragma: export

namespace exy
{
struct adapter
{
    friend constexpr auto operator|(exy::future auto&& lhs, std::derived_from<adapter> auto&& self)
    {
        return exy_fwd(self)(exy_fwd(lhs));
    }
};

template <typename Fn>
struct adapter_proxy : Fn, adapter
{
    explicit adapter_proxy(Fn&& fn) : Fn(exy_fwd(fn)) {}
};

template <typename Tag>
constexpr auto make_adaptor_proxy(exy::movable_object auto&&... args)
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
    template <typename S>
    static constexpr void* call(exy::state_ref s, exy::storage_ref result)
        requires requires { Derived::template continuation_for<S>(s, result); }
    {
        EXY_TAIL_CALL Derived::template continuation_for<S>(s, result)(s, result);
    }

    template <typename S>
    static constexpr void* call(exy::state_ref s, exy::storage_ref result)
    {
        EXY_TAIL_CALL Cont::template call<S>(s, result);
    }
};
} // namespace exy

#endif // EXY_SUPPORT_ADAPTER_HPP_INCLUDED

