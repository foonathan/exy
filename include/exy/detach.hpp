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
    struct _c
    {
        exy::storage<F::storage_spec()> _result;

        static constexpr _c&  _get_self(exy::ctx_base& ctx) noexcept;
        static constexpr void _destroy(exy::ctx_base& ctx) noexcept;

        static constexpr F&                 get_future(exy::ctx_base& ctx) noexcept;
        static constexpr exy::op_of<F, _c>& get_op(exy::ctx_base& ctx) noexcept;

        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            return exy::storage_ref(_get_self(ctx)._result);
        }

        static constexpr auto query(exy::query auto, auto&&...) noexcept
        {
            return exy::no_such_query{};
        }

        template <typename S>
        static constexpr void* call(exy::ctx_base& ctx)
        {
            _c& self = _get_self(ctx);
            (void)exy::storage_ref(self._result).get<S>(); // destroy
            if constexpr (exy::signature_with_tag<S, exy::error_tag>)
                std::terminate(); // unhandled exception

            _destroy(ctx);
            return nullptr;
        }
    };

    template <typename F>
    struct _ctx : exy::ctx_base, _c<F>
    {
        F                    _f;
        exy::op_of<F, _c<F>> _op;

        constexpr explicit _ctx(F&& f) noexcept : _f(exy_mov(f)) {}
    };

    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::void_value_signature<S>
    static constexpr void operator()(F&& f)
    {
        auto ctx = new _ctx(exy_mov(f));
        ctx->_op.start(*ctx);
    }
} detach;

template <typename F>
constexpr detach_t::_c<F>& detach_t::_c<F>::_get_self(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx);
}

template <typename F>
constexpr void detach_t::_c<F>::_destroy(exy::ctx_base& ctx) noexcept
{
    delete &static_cast<_ctx<F>&>(ctx);
}

template <typename F>
constexpr F& detach_t::_c<F>::get_future(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx)._f;
}

template <typename F>
constexpr exy::op_of<F, detach_t::_c<F>>& detach_t::_c<F>::get_op(exy::ctx_base& ctx) noexcept
{
    return static_cast<_ctx<F>&>(ctx)._op;
}
} // namespace exy

#endif // EXY_DETACH_HPP_INCLUDED

