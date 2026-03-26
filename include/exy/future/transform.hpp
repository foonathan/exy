// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_TRANSFORM_HPP_INCLUDED
#define EXY_FUTURE_TRANSFORM_HPP_INCLUDED

#include <exy/support/adapter.hpp>
#include <exy/support/invoke.hpp>

namespace exy
{
template <typename Fn, typename S, typename Tag>
using invoke_results_of_signature_tag = _::mp_unique<exy::signatures_fold_tag<
    S, Tag, _::mp_quote<_::mp_list>, _::mp_bind_front<exy::invoke_result_t, Fn>>>;

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
            // And we must be able to nothrow move construct the result into the storage.
            && exy::signatures_all_of_tag<
                _transformed_values, TagTo,
                _::mp_compose<exy::pack, std::is_nothrow_move_constructible>>>;

    using state = exy::state_of<Base>;

    static consteval auto storage_spec() noexcept
    {
        return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
    }

    template <typename Cont>
    struct op
    {
        struct _c : exy::adapter_continuation<_c, Cont>
        {
            static constexpr Base& get_future(exy::state_base& s) noexcept
            {
                return Cont::get_future(s)._base;
            }

            template <exy::signature_with_tag<TagFrom> S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                _t&              self   = Cont::get_future(s);
                exy::storage_ref result = Cont::get_result_storage(s);

                using transformed_type
                    = exy::signature_arguments_as<S, _::mp_bind_front<exy::invoke_result_t, Fn&&>>;

                try
                {
                    auto [... args] = result.get<S>();
                    if constexpr (std::is_void_v<transformed_type>)
                    {
                        exy_invoke(exy_mov(self)._fn, exy_fwd(args)...);
                        return exy::set<signatures, Cont, TagTo()>(result);
                    }
                    else
                    {
                        return exy::set<signatures, Cont, TagTo(transformed_type)>(
                            result, exy_invoke(exy_mov(self)._fn, exy_fwd(args)...)
                        );
                    }
                }
                catch (...)
                {
                    return exy::set_exception<signatures, Cont>(result);
                }
            }
        };

        static constexpr void* start(exy::state_base& s)
        {
            EXY_TAIL_CALL Base::template op<_c>::start(s);
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

inline constexpr transform_t<exy::value_tag, exy::value_tag>     transform;
inline constexpr transform_t<exy::error_tag, exy::error_tag>     transform_error;
inline constexpr transform_t<exy::stopped_tag, exy::stopped_tag> transform_stopped;

inline constexpr transform_t<exy::error_tag, exy::value_tag>   upon_error;
inline constexpr transform_t<exy::stopped_tag, exy::value_tag> upon_stopped;
} // namespace exy::futures

#endif // EXY_FUTURE_TRANSFORM_HPP_INCLUDED

