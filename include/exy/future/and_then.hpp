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
concept invocable_with_signature_tag_yielding_future
    = exy::invocable_with_signature_tag<Fn, S, Tag>
   && _::mp_all_of<exy::invoke_results_of_signature_tag<Fn, S, Tag>, exy::is_future>::value;
} // namespace exy

namespace exy::futures
{
template <typename Base, typename Tag, typename Fn>
struct _at : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    using _fn_result_types
        = exy::invoke_results_of_signature_tag<Fn, exy::signatures_of<Base>, Tag>;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_replace_tag<
            exy::signatures_of<Base>, Tag,
            _::mp_flatten<_::mp_rename<
                _::mp_transform<exy::signatures_of, _fn_result_types>, exy::signatures>>>,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, Tag, _::mp_bind_front<exy::is_nothrow_invocable, Fn>>>;

    struct state : exy::state_base
    {
        _::mp_apply<std::variant, _::mp_set_push_front<_fn_result_types, std::monostate>>
            _sub_future;
        _::mp_apply<
            std::variant,
            _::mp_set_push_front<
                _::mp_transform<exy::state_of, _fn_result_types>, exy::state_of<Base>>>
            _sub_state;
    };

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
            static constexpr Sub& get_future(exy::state_base& s) noexcept
            {
                return std::get<Sub>(Cont::get_state(s)._sub_future);
            }
            static constexpr exy::state_of<Sub>& get_state(exy::state_base& s) noexcept
            {
                return std::get<exy::state_of<Sub>>(Cont::get_state(s)._sub_state);
            }
        };

        struct _cb : exy::adapter_continuation<_cb, Cont>
        {
            static constexpr Base& get_future(exy::state_base& s) noexcept
            {
                return Cont::get_future(s)._base;
            }
            static constexpr exy::state_of<Base>& get_state(exy::state_base& s) noexcept
            {
                return std::get<exy::state_of<Base>>(Cont::get_state(s)._sub_state);
            }

            template <exy::signature_with_tag<Tag> S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                _at&             self   = Cont::get_future(s);
                state&           state  = Cont::get_state(s);
                exy::storage_ref result = Cont::get_result_storage(s);

                try
                {
                    auto [... args] = result.get<S>();

                    using sub_t = exy::invoke_result_t<Fn, decltype(args)...>;
                    state._sub_future.template emplace<sub_t>(
                        exy_invoke(exy_mov(self)._fn, exy_mov(args)...)
                    );
                    state._sub_state.template emplace<exy::state_of<sub_t>>();

                    return &sub_t::template op<_cs<sub_t>>::start;
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(result);
                }
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            EXY_TAIL_CALL Base::template op<_cb>::start(s);
        }
    };
};

template <typename Tag>
struct and_then_t
{
    template <
        exy::future F, typename S = exy::signatures_of<F>,
        exy::invocable_with_signature_tag_yielding_future<S, Tag> Fn>
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

