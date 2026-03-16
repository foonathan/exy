// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_TRANSFORM_HPP_INCLUDED
#define EXY_FUTURE_TRANSFORM_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
template <typename Fn, typename S>
concept invocable_with_signature_values
    = exy::signatures_all_of_tag<S, exy::value_tag, _::mp_bind_front<exy::is_invocable, Fn>>;
} // namespace exy

namespace exy::futures
{
template <typename Base, typename Fn>
struct _t : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    using _transformed_values = exy::signatures_transform_tag<
        exy::signatures_of<Base>, exy::value_tag, _::mp_bind_front<exy::invoke_result_t, Fn>>;
    using signatures = exy::signatures_insert_exception<
        _transformed_values,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, exy::value_tag,
            _::mp_bind_front<exy::is_nothrow_invocable, Fn>>
            // And we must be able to nothrow move construct it into the storage.
            && exy::signatures_all_of_tag<
                _transformed_values, exy::value_tag,
                _::mp_bind_front<std::is_nothrow_move_constructible>>>;

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
        struct _c
        {
            template <typename S>
            static constexpr void* call(exy::state_ref s, exy::storage_ref result)
            {
                state& self = s.get<Path...>();

                if constexpr (std::same_as<exy::signature_tag<S>, exy::value_tag>)
                {
                    using value_type       = exy::signature_argument<S>;
                    using transformed_type = exy::invoke_result_t<Fn&&, value_type>;
                    EXY_TAIL_CALL exy::set_value_or_exception<Cont, transformed_type>(result, [&] {
                        return exy_invoke(exy_mov(self._fn), result.get<value_type>());
                    })(s, result);
                }
                else
                {
                    EXY_TAIL_CALL Cont::template call<S>(s, result);
                }
            }
        };

        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            EXY_TAIL_CALL Base::template op<_c, Path..., &state::_base>::start(s, result);
        }
    };
};

inline constexpr struct transform_t
{
    template <
        exy::future                             F, typename S = exy::signatures_of<F>,
        exy::invocable_with_signature_values<S> Fn>
    static constexpr auto operator()(F&& f, Fn&& fn) -> _t<F, std::decay_t<Fn>>
    {
        return {{}, exy_mov(f), exy_fwd(fn)};
    }
} transform;
} // namespace exy::futures

#endif // EXY_FUTURE_TRANSFORM_HPP_INCLUDED

