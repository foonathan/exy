// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_ALL_HPP_INCLUDED
#define EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

#include <exy/future/_when.hpp>

namespace exy::futures
{
template <typename... F>
struct _wall : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS exy::pack<F...> _base;

    using _value_signature = _::mp_apply<
        exy::value_tag::make, _::mp_append<exy::signatures_fold_tag<
                                  exy::signatures_of<F>, exy::value_tag,
                                  _::mp_quote<std::type_identity_t>, _::mp_quote<_::mp_list>>...>>;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_replace_tag<
            _::mp_append<exy::signatures<>, exy::signatures_of<F>...>, exy::value_tag,
            exy::signatures<_value_signature>>,
        std::is_nothrow_move_constructible_v<
            exy::signature_arguments_as<_value_signature, _::mp_quote<exy::pack>>>>;

    static consteval auto storage_spec() noexcept
    {
        // Intermediate storage lives in state.
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    using op = _when_op<_wall, Cont>;

    template <typename S>
    static consteval bool _stop_on() noexcept
    {
        return !exy::signature_with_tag<S, exy::value_tag>;
    }

    template <typename Cont>
    static constexpr exy::continuation _on_complete(exy::ctx_base& ctx) noexcept
    {
        op<Cont>& self = Cont::get_op(ctx);

        if (auto cont = self._continuation.load(std::memory_order_relaxed))
            // We already have a failure, call it.
            return cont;

        auto result = Cont::get_result_storage(ctx);
        try
        {
            auto& [... storage] = self._storage;
            auto [... args]     = std::tuple_cat([&] {
                using signature
                    = _::mp_only<_::mp_filter<exy::value_tag::is, exy::signatures_of<F>>>;
                auto [... args] = exy::storage_ref(storage).get<signature>();
                return std::make_tuple(exy_mov(args)...);
            }()...);
            return exy::set<signatures, Cont, _value_signature>(result, exy_mov(args)...);
        }
        catch (...)
        {
            return exy::set_exception<signatures, Cont>(result);
        }
    }
};

inline constexpr struct when_all_t
{
    template <exy::future F, typename S = exy::signatures_of<F>>
        requires exy::single_value_signatures<S>
    static constexpr F&& operator()(F&& f)
    {
        return exy_mov(f);
    }

    template <exy::future... F>
        requires (sizeof...(F) > 1) && (exy::single_value_signatures<exy::signatures_of<F>> && ...)
    static constexpr _wall<F...> operator()(F&&... f)
    {
        return {{}, exy::make_pack(exy_mov(f)...)};
    }
} when_all;
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

