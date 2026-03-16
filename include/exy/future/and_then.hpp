// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

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

    template <typename F>
    using _state_is_nothrow_constructible = std::is_nothrow_constructible<exy::state_of<F>, F&&>;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_merge_on_tag<
            exy::signatures_of<Base>, Tag,
            _::mp_flatten<_::mp_rename<
                _::mp_transform<exy::signatures_of, _fn_result_types>, exy::signatures>>>,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, Tag, _::mp_bind_front<exy::is_nothrow_invocable, Fn>>
            // And we must be able to nothrow move construct the state.
            && _::mp_all_of<_fn_result_types, _state_is_nothrow_constructible>::value>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS typename Base::state _base;
        EXY_NO_UNIQUE_ADDRESS Fn                   _fn;
        _::mp_apply<
            std::variant,
            _::mp_set_push_front<_::mp_transform<exy::state_of, _fn_result_types>, std::monostate>>
            _inner;

        constexpr explicit state(_at&& self) : _base(exy_mov(self)._base), _fn(exy_mov(self)._fn) {}
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

    template <typename Cont, auto... Path>
    struct op
    {
        struct _c : exy::adapter_continuation<_c, Cont>
        {
            template <exy::signature_with_tag<Tag> S>
            static constexpr exy::continuation continuation_for(
                exy::state_ref s, exy::storage_ref result
            ) noexcept
            {
                state& self = s.get<Path...>();

                return result.get<S>([&](auto&&... args) {
                    try
                    {
                        auto inner          = exy_invoke(self._fn, exy_fwd(args)...);
                        using inner_t       = decltype(inner);
                        using inner_state_t = exy::state_of<inner_t>;

                        self._inner.template emplace<inner_state_t>(exy_mov(inner));

                        static constexpr auto inner_path
                            = [](exy::state_base* s) -> inner_state_t& {
                            return std::get<inner_state_t>(static_cast<state*>(s)->_inner);
                        };
                        return &inner_t::template op<Cont, Path..., inner_path>::start;
                    }
                    catch (...)
                    {
                        return exy::set_exception<signatures, Cont>(result);
                    }
                });
            }
        };

        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            EXY_TAIL_CALL Base::template op<_c, Path..., &state::_base>::start(s, result);
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
inline constexpr and_then_t<exy::error_tag>   and_then_error;
inline constexpr and_then_t<exy::stopped_tag> and_then_stopped;
} // namespace exy::futures

#endif // EXY_FUTURE_AND_THEN_HPP_INCLUDED

