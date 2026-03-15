// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_TRANSFORM_HPP_INCLUDED
#define EXY_FUTURE_TRANSFORM_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
template <typename Fn, auto Signature>
concept invocable_with_signature_values
    = []<typename... T>(exy::signature_t<exy::signature_value_t(T)...>) {
          return (exy::invocable<Fn&&, T> && ...);
      }(Signature);
} // namespace exy

namespace exy::futures
{
template <typename Base, typename Fn>
struct _t : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    static consteval auto signature() noexcept
    {
        return []<typename... T>(exy::signature_t<exy::signature_value_t(T)...>) {
            return exy::signature_t<exy::signature_value_t(exy::invoke_result_t<Fn&&, T>)...>{};
        }(Base::signature());
    }

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS typename Base::state _base;
        EXY_NO_UNIQUE_ADDRESS Fn                   _fn;

        constexpr explicit state(_t&& self) : _base(exy_mov(self)._base), _fn(exy_mov(self)._fn) {}
    };

    static consteval auto storage_spec() noexcept
    {
        return exy::storage_spec::get(Base::signature(), signature());
    }

    template <typename Cont, auto... Path>
    struct op
    {
        struct _c
        {
            template <typename T>
            static constexpr void* resume(exy::state_ref s, exy::storage_ref result)
            {
                state& self = s.get<Path...>();

                using transformed_t = exy::invoke_result_t<Fn&&, T&&>;
                result.emplace<transformed_t>(exy_invoke(exy_mov(self._fn), result.get<T>()));
                EXY_TAIL_CALL Cont::template resume<transformed_t>(s, result);
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
    template <exy::future F, auto S = F::signature(), exy::invocable_with_signature_values<S> Fn>
    static constexpr auto operator()(F&& f, Fn&& fn) EXY_RETURN(
        _t<F, std::decay_t<Fn>>{{}, exy_mov(f), exy_fwd(fn)}
    )
} transform;
} // namespace exy::futures

#endif // EXY_FUTURE_TRANSFORM_HPP_INCLUDED

