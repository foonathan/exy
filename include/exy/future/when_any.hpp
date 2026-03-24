// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_ANY_HPP_INCLUDED
#define EXY_FUTURE_WHEN_ANY_HPP_INCLUDED

#include <atomic>
#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

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

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::pack<exy::state_of<F>...> _base;
        exy::pack<exy::storage<F::storage_spec()>...>        _storage;
        std::atomic<unsigned>                                _done = 0;
        exy::continuation                                    _continuation;

        constexpr explicit state(_wany& self) noexcept(
            (exy::has_nothrow_constructible_state<F> && ...)
        )
        : _base([&] {
              auto& [... f] = self._base;
              return decltype(_base)(f...);
          }())
        {}
    };

    static consteval auto storage_spec() noexcept
    {
        // Intermediate storage lives in state.
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont>
    struct op
    {
        template <std::size_t Idx>
        struct _c : exy::adapter_continuation<_c<Idx>, Cont>
        {
            static constexpr auto& get_future(exy::state_base& s) noexcept
            {
                return std::get<Idx>(Cont::get_future(s)._base);
            }
            static constexpr auto& get_state(exy::state_base& s) noexcept
            {
                return std::get<Idx>(Cont::get_state(s)._base);
            }
            static constexpr exy::storage_ref get_result_storage(exy::state_base& s) noexcept
            {
                return exy::storage_ref(std::get<Idx>(Cont::get_state(s)._storage));
            }

            static constexpr bool override_query(
                exy::queries::stop_requested_t q, exy::state_base& s
            )
            {
                state& self = Cont::get_state(s);
                if (self._done.load(std::memory_order_relaxed) > 0)
                    return true;

                if constexpr (
                    exy::has_query<Cont, exy::queries::stop_requested_t, exy::state_base&>
                )
                    EXY_TAIL_CALL Cont::query(q, s);
                else
                    return false;
            }

            template <typename S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                state&           self   = Cont::get_state(s);
                exy::storage_ref result = Cont::get_result_storage(s);

                exy::storage_ref base_result = get_result_storage(s);
                auto [... args]              = base_result.get<S>();

                auto prev_count = self._done.fetch_add(1, std::memory_order_acq_rel);
                if (prev_count == 0)
                {
                    // First one to complete, store the result.
                    self._continuation = exy::set<signatures, Cont, S>(result, exy_mov(args)...);
                }

                if (prev_count + 1 == sizeof...(F))
                    // Last to complete, forward the actual result.
                    return self._continuation;
                else
                    // Still have to wait for more to complete.
                    return nullptr;
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            state& self = Cont::get_state(s);

            return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
                return (F::template op<_c<Idx>>::start(s), ...);
            }(std::index_sequence_for<F...>{});
        }
    };
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

