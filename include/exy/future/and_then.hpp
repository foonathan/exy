// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_AND_THEN_HPP_INCLUDED
#define EXY_FUTURE_AND_THEN_HPP_INCLUDED

#include <variant>
#include <exy/future/transform.hpp>
#include <exy/support/adapter.hpp>
#include <exy/support/invoke.hpp>

namespace exy
{
template <typename Fn, typename S, typename Tag>
concept invocable_with_tagged_signatures_yielding_future
    = exy::invocable_with_tagged_signatures<Fn, S, Tag>
   && _::mp_all_of<exy::invoke_results_of_tagged_signatures<Fn, S, Tag>, exy::is_future>::value;
} // namespace exy

namespace exy::futures
{
template <typename Base, typename Tag, typename Fn>
struct _at : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    using _fn_result_types
        = exy::invoke_results_of_tagged_signatures<Fn, exy::signatures_of<Base>, Tag>;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_replace_tag<
            exy::signatures_of<Base>, Tag,
            _::mp_flatten<_::mp_rename<
                _::mp_transform<exy::signatures_of, _fn_result_types>, exy::signatures>>>,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, Tag, _::mp_bind_front<exy::is_nothrow_invocable, Fn>>>;

    static consteval auto storage_spec() noexcept
    {
        auto result = exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
        _::mp_for_each<_::mp_transform<std::type_identity, _fn_result_types>>(
            [&]<typename Inner>(std::type_identity<Inner>) {
                result = exy::max(result, Inner::storage_spec());
            }
        );
        return result;
    }

    template <typename Cont>
    struct op
    {
        template <typename Sub>
        struct _cs : Cont
        {
            static constexpr Sub& get_future(exy::ctx_base& ctx) noexcept
            {
                return std::get<Sub>(Cont::get_op(ctx)._sub_future);
            }
            static constexpr exy::op_of<Sub, _cs>& get_op(exy::ctx_base& ctx) noexcept
            {
                return std::get<exy::op_of<Sub, _cs>>(Cont::get_op(ctx)._sub_op);
            }
        };
        template <typename Sub>
        using _sub_op_of = exy::op_of<Sub, _cs<Sub>>;

        struct _cb : exy::adapter_continuation<_cb, Cont>
        {
            static constexpr Base& get_future(exy::ctx_base& ctx) noexcept
            {
                return Cont::get_future(ctx)._base;
            }
            static constexpr exy::op_of<Base, _cb>& get_op(exy::ctx_base& ctx) noexcept
            {
                return std::get<exy::op_of<Base, _cb>>(Cont::get_op(ctx)._sub_op);
            }

            template <exy::signature_with_tag<Tag> S>
            static constexpr exy::continuation continuation_for(exy::ctx_base& ctx) noexcept
            {
                op&              op     = Cont::get_op(ctx);
                _at&             f      = Cont::get_future(ctx);
                exy::storage_ref result = Cont::get_result_storage(ctx);

                try
                {
                    return result.with<S>([&](auto&& pack) {
                        auto&& [... args] = exy_mov(pack);

                        using sub_t = exy::invoke_result_t<Fn, decltype(args)...>;
                        op._sub_future.template emplace<sub_t>(
                            exy_invoke(exy_mov(f)._fn, exy_mov(args)...)
                        );

                        return &op._sub_op.template emplace<exy::op_of<sub_t, _cs<sub_t>>>().start;
                    });
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(result);
                }
            }
        };

        _::mp_apply<std::variant, _::mp_set_push_front<_fn_result_types, std::monostate>>
            _sub_future;
        _::mp_apply<
            std::variant, _::mp_set_push_front<
                              _::mp_transform<_sub_op_of, _fn_result_types>, exy::op_of<Base, _cb>>>
            _sub_op;

        static constexpr void* start(exy::ctx_base& ctx)
        {
            op&           self    = Cont::get_op(ctx);
            auto&         base_op = self._sub_op.template emplace<exy::op_of<Base, _cb>>();
            EXY_TAIL_CALL base_op.start(ctx);
        }
    };
};

template <typename Tag>
struct and_then_t
{
    template <
        exy::future F, typename S = exy::signatures_of<F>,
        exy::invocable_with_tagged_signatures_yielding_future<S, Tag> Fn>
    static constexpr auto operator()(F&& f, Fn&& fn) -> _at<F, Tag, std::decay_t<Fn>>
    {
        return {{}, exy_mov(f), exy_fwd(fn)};
    }

    template <exy::movable Fn>
    static constexpr auto operator()(Fn&& fn)
    {
        return exy::make_adaptor_proxy<and_then_t>(exy_fwd(fn));
    }
};

inline constexpr and_then_t<exy::value_tag>   and_then;
inline constexpr and_then_t<exy::error_tag>   or_else_error;
inline constexpr and_then_t<exy::stopped_tag> or_else_stopped;
} // namespace exy::futures

#endif // EXY_FUTURE_AND_THEN_HPP_INCLUDED

