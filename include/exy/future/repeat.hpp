// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_REPEAT_HPP_INCLUDED
#define EXY_FUTURE_REPEAT_HPP_INCLUDED

#include <optional>
#include <exy/future/yield.hpp>
#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Tag, typename Base>
struct _r : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_replace_tag<
            exy::signatures_of<Base>, Tag, exy::signatures<exy::stopped_tag()>>,
        std::is_nothrow_copy_constructible_v<Base>>;

    static consteval auto storage_spec() noexcept
    {
        return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
    }

    template <typename Cont>
    struct op
    {
        struct _c;

        struct _simple_state : exy::ctx_base
        {
            EXY_NO_UNIQUE_ADDRESS Base future;
            EXY_NO_UNIQUE_ADDRESS exy::op_of<Base, _c> op;
        };

        struct _optional_state : exy::ctx_base
        {
            struct impl
            {
                EXY_NO_UNIQUE_ADDRESS Base future;
                EXY_NO_UNIQUE_ADDRESS exy::op_of<Base, _c> op;

                explicit impl(const Base& base) : future(base) {}
            };
            std::optional<impl> impl;
        };

        std::conditional_t<
            std::is_default_constructible_v<Base> && std::is_copy_assignable_v<Base>
                && std::is_move_assignable_v<exy::op_of<Base, _c>>,
            _simple_state, _optional_state>
            _state;

        static constexpr exy::continuation _restart(exy::ctx_base& ctx) noexcept
        {
            op&              self   = Cont::get_op(ctx);
            _r&              f      = Cont::get_future(ctx);
            exy::storage_ref result = Cont::get_result_storage(ctx);

            if (exy::query_or_default<Cont>(queries::stop_requested, ctx))
                return exy::set<signatures, Cont, exy::stopped_tag()>(result);

            try
            {
                if constexpr (std::same_as<decltype(_state), _simple_state>)
                {
                    self._state.future = f._base;
                    self._state.op     = {};
                    return &self._state.op.start;
                }
                else
                {
                    self._state.impl.emplace(f._base);
                    return &self._state.impl->op.start;
                }
            }
            catch (...)
            {
                return exy::set_exception<signatures, Cont>(result);
            }
        }

        struct _c : exy::adapter_continuation<_c, Cont>
        {
            static constexpr Base& get_future(exy::ctx_base& ctx) noexcept
            {
                if constexpr (std::same_as<decltype(_state), _simple_state>)
                    return Cont::get_op(ctx)._state.future;
                else
                    return Cont::get_op(ctx)._state.impl->future;
            }
            static constexpr exy::op_of<Base, _c>& get_op(exy::ctx_base& ctx) noexcept
            {
                if constexpr (std::same_as<decltype(_state), _simple_state>)
                    return Cont::get_op(ctx)._state.op;
                else
                    return Cont::get_op(ctx)._state.impl->op;
            }

            template <exy::signature_with_tag<Tag> S>
            static constexpr exy::continuation continuation_for(exy::ctx_base& ctx) noexcept
            {
                exy::storage_ref result = Cont::get_result_storage(ctx);
                (void)result.get<S>(); // destroy
                return _restart(ctx);
            }
        };

        static constexpr void* start(exy::ctx_base& ctx)
        {
            EXY_TAIL_CALL _restart(ctx)(ctx);
        }
    };
};

template <typename Tag>
struct eager_repeat_t : exy::adapter
{
    template <exy::future F, typename S = exy::signatures_of<F>>
        requires std::is_copy_constructible_v<F>
    static constexpr _r<Tag, F> operator()(F&& f)
    {
        return {{}, exy_mov(f)};
    }
};

inline constexpr eager_repeat_t<exy::value_tag> eager_repeat;
inline constexpr eager_repeat_t<exy::error_tag> eager_retry;

template <typename Tag>
struct repeat_t : exy::adapter
{
    template <exy::future F, typename S = exy::signatures_of<F>>
        requires std::is_copy_constructible_v<F>
    static constexpr auto operator()(F&& f)
    {
        return exy::futures::eager_repeat_t<Tag>{}(exy::futures::yield(exy_mov(f)));
    }
};

inline constexpr repeat_t<exy::value_tag> repeat;
inline constexpr repeat_t<exy::error_tag> retry;
} // namespace exy::futures

#endif // EXY_FUTURE_REPEAT_HPP_INCLUDED

