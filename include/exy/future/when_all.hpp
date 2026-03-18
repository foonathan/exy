// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_FUTURE_WHEN_ALL_HPP_INCLUDED
#define EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

#include <atomic>
#include <tuple>
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
        exy::storage_ref                                     _result;
        std::atomic<exy::continuation>                       _error_continuation = nullptr;
        std::atomic<unsigned>                                _done               = 0;

        constexpr explicit state(_wall&& self) noexcept(
            (std::is_nothrow_constructible_v<exy::state_of<F>, F&&> && ...)
        )
        : _base([&] {
              auto [... f] = exy_mov(self)._base;
              return exy::make_pack(exy::state_of<F>(exy_mov(f))...);
          }())
        {}

        template <typename Cont>
        exy::continuation complete(exy::storage_ref& result) noexcept
        {
            if (_done.fetch_add(1, std::memory_order_acq_rel) + 1 < sizeof...(F))
                return nullptr;

            result = _result;
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
                return exy::set<signatures, Cont, _value_signature>(_result, exy_mov(args)...);
            }
            catch (...)
            {
                return exy::set_exception<signatures, Cont>(_result);
            }
        }
    };

    static consteval auto storage_spec() noexcept
    {
        // Intermediate storage lives in state.
        return exy::storage_spec::get(signatures());
    }

    template <typename Cont, auto... Path>
    struct op
    {
        struct _c : exy::adapter_continuation<_c, Cont>
        {
            template <exy::signature_with_tag<exy::value_tag> S>
            static constexpr exy::continuation continuation_for(
                exy::state_ref s, exy::storage_ref& result
            ) noexcept
            {
                state& self = s.get<Path...>();
                return self.template complete<Cont>(result);
            }

            template <typename S> // error or stopped
            static constexpr exy::continuation continuation_for(
                exy::state_ref s, exy::storage_ref& result
            ) noexcept
            {
                state& self = s.get<Path...>();

                if (self._error_continuation.exchange(
                        &Cont::template call<S>, std::memory_order_relaxed
                    )
                    == nullptr)
                {
                    // We are the first failure, properly set it.
                    auto [... args] = result.get<S>();
                    self._result.template emplace<S>(exy_mov(args)...);
                }

                return self.template complete<Cont>(result);
            }
        };

        template <std::size_t Idx>
        static consteval auto _path_for() noexcept
        {
            return [](state_base* s) -> auto& {
                state& self = *static_cast<state*>(s);
                return std::get<Idx>(self._base);
            };
        }

        static constexpr void* start(exy::state_ref s, exy::storage_ref result)
        {
            if constexpr (sizeof...(F) == 0)
            {
                auto          cont = exy::set<signatures, Cont, _value_signature>(result);
                EXY_TAIL_CALL cont(s, result);
            }
            else
            {
                state& self  = s.get<Path...>();
                self._result = result;

                return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
                    auto& [... storage] = self._storage;
                    return (
                        F::template op<_c, Path..., _path_for<Idx>()>::start(
                            s, exy::storage_ref(storage)
                        ),
                        ...
                    );
                }(std::index_sequence_for<F...>{});
            }
        }
    };
};

inline constexpr struct when_all_t
{
    template <exy::future... F>
        requires (exy::single_value_signatures<exy::signatures_of<F>> && ...)
    static constexpr _wall<F...> operator()(F&&... f)
    {
        return {{}, exy::make_pack(exy_mov(f)...)};
    }
} when_all;
} // namespace exy::futures

#endif // EXY_FUTURE_WHEN_ALL_HPP_INCLUDED

