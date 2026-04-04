// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_ANY_HPP_INCLUDED
#define EXY_FUTURE_WHEN_ANY_HPP_INCLUDED

#include <exy/future/_when.hpp>

namespace exy::futures
{
template <typename... F>
struct _wany : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS exy::pack<F...> _base;

    using _combined_signatures = _::mp_unique<_::mp_append<exy::signatures_of<F>...>>;
    using signatures           = exy::signatures_insert_exception<
        _combined_signatures,
        // All values have to be nothrow move constructible.
        exy::signatures_all_of_tag<
            _combined_signatures, exy::any_tag,
            _::mp_compose<exy::pack, std::is_nothrow_move_constructible>>>;

    static consteval auto storage_spec() noexcept
    {
        // Intermediate storage lives in state.
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    using op = _when_op<_wany, Cont>;

    template <typename S>
    static consteval bool _stop_on() noexcept
    {
        return true;
    }

    template <typename Cont>
    static constexpr exy::continuation _on_complete(exy::ctx_base& ctx) noexcept
    {
        op<Cont>& self = Cont::get_op(ctx);
        return self._continuation.load(std::memory_order_relaxed);
    }
};

inline constexpr struct when_any_t
{
    template <exy::future F>
    static constexpr F&& operator()(F&& f)
    {
        return exy_mov(f);
    }

    template <exy::future... F>
        requires (sizeof...(F) > 1)
    static constexpr _wany<F...> operator()(F&&... f)
    {
        return {{}, exy::make_pack(exy_mov(f)...)};
    }
} when_any;
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_ANY_HPP_INCLUDED

