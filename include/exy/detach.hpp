// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_DETACH_HPP_INCLUDED
#define EXY_DETACH_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/query.hpp>

namespace exy
{
inline constexpr struct detach_t
{
    template <typename F>
    struct _c : exy::ctx_base
    {
        F                               _f;
        exy::op_of<F, _c>               _op;
        exy::storage<F::storage_spec()> _result;

        constexpr explicit _c(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>)
        : _f(exy_mov(f))
        {}

        static constexpr F& get_future(exy::ctx_base& ctx) noexcept
        {
            return static_cast<_c&>(ctx)._f;
        }
        static constexpr exy::op_of<F, _c>& get_op(exy::ctx_base& ctx) noexcept
        {
            return static_cast<_c<F>&>(ctx)._op;
        }
        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            return exy::storage_ref(static_cast<_c&>(ctx)._result);
        }

        static constexpr auto query(exy::query auto, exy::ctx_base&) noexcept
        {
            return exy::no_such_query{};
        }

        template <typename S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            auto self = static_cast<_c*>(&ctx);
            (void)exy::storage_ref(self->_result).get<S>(); // destroy
            if constexpr (exy::signature_with_tag<S, exy::error_tag>)
                std::terminate(); // unhandled exception

            delete self;
            return nullptr;
        }
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::void_value_signature<S>
    static constexpr void operator()(F&& f)
    {
        auto ctx = new _c(exy_mov(f));
        ctx->_op.start(*ctx);
    }
} detach;
} // namespace exy

#endif // EXY_DETACH_HPP_INCLUDED

