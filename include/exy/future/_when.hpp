// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_HPP_INCLUDED
#define EXY_FUTURE_WHEN_HPP_INCLUDED

#include <atomic>
#include <exy/query/scheduler.hpp>
#include <exy/query/stop.hpp>
#include <exy/query/work.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Derived, typename Cont>
struct _when_op;
template <template <typename...> typename Derived, typename... F, typename Cont>
struct _when_op<Derived<F...>, Cont>
{
    struct _s : exy::scheduler_base
    {
        _when_op* _self;

        struct _f : exy::future_base
        {
            _when_op* _self;

            using signatures = exy::signatures<exy::value_tag()>;

            static consteval auto storage_spec() noexcept
            {
                return exy::storage_spec::get(signatures());
            }

            template <typename ContS>
            struct op
            {
                static constexpr void* start(exy::ctx_base& ctx)
                {
                    _when_op&        self   = *ContS::get_future(ctx)._self;
                    exy::storage_ref result = ContS::get_result_storage(ctx);

                    if (auto cont
                        = self._yield(exy::set<signatures, ContS, exy::value_tag()>(result)))
                        EXY_TAIL_CALL cont(ctx);
                    else
                        return nullptr;
                }
            };
        };

        constexpr _f schedule() const noexcept
        {
            return {{}, _self};
        }
    };

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

        static constexpr bool query(exy::queries::stop_requested_t q, exy::ctx_base& ctx)
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

        static constexpr _s query(exy::queries::delegation_scheduler_t, exy::ctx_base& ctx)
        {
            _when_op& self = Cont::get_op(ctx);
            return {{}, &self};
        }

        static constexpr exy::continuation query(exy::queries::inline_work_t, exy::ctx_base& ctx)
        {
            _when_op& self = Cont::get_op(ctx);
            return self._yield(nullptr);
        }

        static constexpr auto query(exy::query auto q, exy::ctx_base& ctx)
            -> decltype(Cont::query(q, ctx))
        {
            return Cont::query(q, ctx);
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
                return self._yield(nullptr);
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
    std::atomic<exy::continuation> _scheduler_queue[sizeof...(F) - 1] = {};
    std::atomic<exy::continuation> _continuation                      = nullptr;
    std::atomic<unsigned>          _done                              = 0;
    std::atomic<unsigned>          _scheduler_idx                     = 0;

    exy::continuation _yield(exy::continuation next)
    {
        for (auto i = 0; i != sizeof...(F); ++i)
        {
            auto idx = _scheduler_idx.fetch_add(1, std::memory_order_acq_rel) % (sizeof...(F) - 1);
            auto expected = _scheduler_queue[idx].load(std::memory_order_relaxed);
            if (expected != nullptr
                && _scheduler_queue[idx].compare_exchange_strong(
                    expected, next, std::memory_order_relaxed
                ))
                return expected;
        }
        return nullptr;
    }

    static constexpr void* start(exy::ctx_base& ctx)
    {
        _when_op& self = Cont::get_op(ctx);

        [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
            ((self._scheduler_queue[Idx].store(
                 &std::get<Idx + 1>(self._base).start, std::memory_order_relaxed
             )),
             ...);
        }(std::make_index_sequence<sizeof...(F) - 1>{});

        EXY_TAIL_CALL std::get<0>(self._base).start(ctx);
    }
};
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_HPP_INCLUDED

