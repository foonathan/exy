// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WHEN_ALL_HPP_INCLUDED
#define EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

#include <atomic>
#include <exy/query/stop.hpp>
#include <exy/support/adapter.hpp>

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

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::pack<exy::state_of<F>...> _base;
        exy::pack<exy::storage<F::storage_spec()>...>        _storage;
        std::atomic<exy::continuation>                       _error_continuation = nullptr;
        std::atomic<unsigned>                                _done               = 0;

        constexpr explicit state(_wall& self) noexcept(
            (exy::has_nothrow_constructible_state<F> && ...)
        )
        : _base([&] {
              auto& [... f] = self._base;
              return decltype(_base)(f...);
          }())
        {}

        template <typename Cont>
        exy::continuation complete(exy::storage_ref result) noexcept
        {
            if (_done.fetch_add(1, std::memory_order_acq_rel) + 1 < sizeof...(F))
                return nullptr;

            if (auto cont = _error_continuation.load(std::memory_order_relaxed))
                return cont;

            try
            {
                auto& [... storage] = _storage;
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
                auto& self = Cont::get_state(s);
                if (self._error_continuation.load(std::memory_order_relaxed) != nullptr)
                    return true;

                if constexpr (
                    exy::has_query<Cont, exy::queries::stop_requested_t, exy::state_base&>
                )
                    EXY_TAIL_CALL Cont::query(q, s);
                else
                    return false;
            }

            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                state& self = Cont::get_state(s);
                return self.template complete<Cont>(Cont::get_result_storage(s));
            }

            template <typename S> // error or stopped
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                state&           self        = Cont::get_state(s);
                exy::storage_ref result      = Cont::get_result_storage(s);
                exy::storage_ref base_result = get_result_storage(s);

                exy::continuation expected = nullptr;
                if (self._error_continuation.compare_exchange_strong(
                        expected, &Cont::template call<S>, std::memory_order_relaxed,
                        std::memory_order_relaxed
                    ))
                {
                    // We are the first failure, properly set it.
                    auto [... args] = base_result.get<S>();
                    result.template emplace<S>(exy_mov(args)...);
                }

                return self.template complete<Cont>(result);
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            exy::storage_ref result = Cont::get_result_storage(s);
            if constexpr (sizeof...(F) == 0)
            {
                auto          cont = exy::set<signatures, Cont, _value_signature>(result);
                EXY_TAIL_CALL cont(s);
            }
            else
            {
                state& self = Cont::get_state(s);

                return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
                    return (F::template op<_c<Idx>>::start(s), ...);
                }(std::index_sequence_for<F...>{});
            }
        }
    };
};

inline constexpr struct when_all_t
{
    template <exy::future F>
        requires exy::single_value_signatures<exy::signatures_of<F>>
    static constexpr F&& operator()(F&& f)
    {
        return exy_mov(f);
    }

    template <exy::future... F>
        requires (exy::single_value_signatures<exy::signatures_of<F>> && ...)
    static constexpr _wall<F...> operator()(F&&... f)
    {
        return {{}, exy::make_pack(exy_mov(f)...)};
    }
} when_all;
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

