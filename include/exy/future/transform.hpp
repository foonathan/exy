// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_TRANSFORM_HPP_INCLUDED
#define EXY_FUTURE_TRANSFORM_HPP_INCLUDED

#include <exy/support/adapter.hpp>
#include <exy/support/base.hpp>

namespace exy
{
template <typename Fn, typename S, typename Tag>
concept invocable_with_signature_tag
    = exy::signatures_all_of_tag<S, Tag, _::mp_bind_front<exy::is_invocable, Fn>>;
} // namespace exy

namespace exy::futures
{
template <typename Base, typename TagFrom, typename Fn, typename TagTo>
struct _t : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    using _transformed_values = exy::signatures_transform_tag<
        exy::signatures_of<Base>, TagFrom, _::mp_bind_front<exy::invoke_result_t, Fn>, TagTo>;
    using signatures = exy::signatures_insert_exception<
        _transformed_values,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, TagFrom, _::mp_bind_front<exy::is_nothrow_invocable, Fn>>
            // And we must be able to nothrow move construct it into the storage.
            && exy::signatures_all_of_tag<
                _transformed_values, TagTo, _::mp_bind_front<std::is_nothrow_move_constructible>>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS typename Base::state _base;
        EXY_NO_UNIQUE_ADDRESS Fn                   _fn;

        constexpr explicit state(_t&& self) : _base(exy_mov(self)._base), _fn(exy_mov(self)._fn) {}
    };

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(exy::signatures_of<Base>{}, signatures());
    }

    template <typename Cont, auto... Path>
    struct op
    {
        struct _c : exy::adapter_continuation<_c, Cont>
        {
            template <exy::signature_with_tag<TagFrom> S>
            static constexpr exy::continuation continuation_for(
                exy::state_ref s, exy::storage_ref result
            ) noexcept
            {
                state& self = s.get<Path...>();

                using value_type       = exy::signature_argument<S>;
                using transformed_type = exy::invoke_result_t<Fn&&, value_type>;
                return exy::set_or_exception<Cont, TagTo(transformed_type)>(result, [&] {
                    return exy_invoke(exy_mov(self._fn), result.get<value_type>());
                });
            }
        };

        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            EXY_TAIL_CALL Base::template op<_c, Path..., &state::_base>::start(s, result);
        }
    };
};

template <typename TagFrom, typename TagTo>
struct transform_t
{
    template <
        exy::future                                   F, typename S = exy::signatures_of<F>,
        exy::invocable_with_signature_tag<S, TagFrom> Fn>
    static constexpr auto operator()(F&& f, Fn&& fn) -> _t<F, TagFrom, std::decay_t<Fn>, TagTo>
    {
        return {{}, exy_mov(f), exy_fwd(fn)};
    }

    template <exy::movable Fn>
    static constexpr auto operator()(Fn&& fn)
    {
        return exy::make_adaptor_proxy<transform_t>(exy_fwd(fn));
    }
};

inline constexpr transform_t<exy::value_tag, exy::value_tag> transform;
inline constexpr transform_t<exy::error_tag, exy::error_tag> transform_error;

inline constexpr transform_t<exy::error_tag, exy::value_tag> upon_error;
} // namespace exy::futures

#endif // EXY_FUTURE_TRANSFORM_HPP_INCLUDED

