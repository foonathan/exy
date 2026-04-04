// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_HPP_INCLUDED
#define EXY_FUTURE_WHEN_HPP_INCLUDED

#include <atomic>
#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Derived, typename Cont>
struct _when_op;
template <template <typename...> typename Derived, typename... F, typename Cont>
struct _when_op<Derived<F...>, Cont>
{
    template <std::size_t Idx>
    struct _c : exy::adapter_continuation<_c<Idx>, Cont>
    {
        static constexpr auto& get_future(exy::ctx_base& ctx) noexcept
        {
            return std::get<Idx>(Cont::get_future(ctx)._base);
        }
        static constexpr auto& get_op(exy::ctx_base& ctx) noexcept
        {
            return std::get<Idx>(Cont::get_op(ctx)._base);
        }
        static constexpr exy::storage_ref get_result_storage(exy::ctx_base& ctx) noexcept
        {
            return exy::storage_ref(std::get<Idx>(Cont::get_op(ctx)._storage));
        }

        static constexpr bool override_query(exy::queries::stop_requested_t q, exy::ctx_base& ctx)
        {
            _when_op& self = Cont::get_op(ctx);
            if (self._continuation.load(std::memory_order_relaxed) != nullptr)
                // We are stopped if we already have a continuation.
                return true;

            if constexpr (exy::has_query<Cont, exy::queries::stop_requested_t, exy::ctx_base&>)
                EXY_TAIL_CALL Cont::query(q, ctx);
            else
                return false;
        }

        template <typename S>
        static constexpr exy::continuation continuation_for(exy::ctx_base& ctx) noexcept
        {
            _when_op& self = Cont::get_op(ctx);

            if constexpr (Derived<F...>::template _stop_on<S>())
            {
                exy::continuation expected = nullptr;
                if (self._continuation.compare_exchange_strong(
                        expected, &Cont::template call<S>, std::memory_order_relaxed,
                        std::memory_order_relaxed
                    ))
                {
                    // We are the first to stop, properly set it.
                    exy::storage_ref result      = Cont::get_result_storage(ctx);
                    exy::storage_ref base_result = get_result_storage(ctx);
                    try
                    {
                        auto [... args] = base_result.get<S>();
                        result.template emplace<S>(exy_mov(args)...);
                    }
                    catch (...)
                    {
                        exy::set_exception<exy::signatures_of<Derived<F...>>, Cont>(result);
                    }
                }
            }

            auto prev_count = self._done.fetch_add(1, std::memory_order_acq_rel);
            if (prev_count + 1 == sizeof...(F))
                return Derived<F...>::template _on_complete<Cont>(ctx);
            else
                return nullptr;
        }
    };

    template <typename Idxs>
    struct _make_op_pack;
    template <std::size_t... Idx>
    struct _make_op_pack<std::index_sequence<Idx...>>
    {
        using type = exy::pack<exy::op_of<F, _c<Idx>>...>;
    };

    EXY_NO_UNIQUE_ADDRESS _make_op_pack<std::index_sequence_for<F...>>::type _base;
    EXY_NO_UNIQUE_ADDRESS exy::pack<exy::storage<F::storage_spec()>...> _storage;
    std::atomic<exy::continuation>                                      _continuation = nullptr;
    std::atomic<unsigned>                                               _done         = 0;

    static constexpr void* start(exy::ctx_base& ctx)
    {
        _when_op& self = Cont::get_op(ctx);

        [&]<typename... FI, std::size_t... Idx>(_::mp_list<FI...>, std::index_sequence<Idx...>) {
            (FI::template op<_c<Idx>>::start(ctx), ...);
        }(_::mp_pop_back<_::mp_list<F...>>{}, std::make_index_sequence<sizeof...(F) - 1>{});
        EXY_TAIL_CALL F...[sizeof...(F) - 1] ::template op<_c<sizeof...(F) - 1>>::start(ctx);
    }
};
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_HPP_INCLUDED

