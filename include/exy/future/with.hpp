// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_FUTURE_WITH_HPP_INCLUDED
#define EXY_FUTURE_WITH_HPP_INCLUDED

#include <exy/future/transform.hpp>
#include <exy/support/adapter.hpp>

namespace exy::futures
{
template <typename Base, typename Tag, typename Fn>
struct _w : exy::future_base
{
    EXY_NO_UNIQUE_ADDRESS Base _base;
    EXY_NO_UNIQUE_ADDRESS Fn   _fn;

    using signatures = exy::signatures_insert_exception<
        exy::signatures_of<Base>,
        // The function must be nothrow.
        exy::signatures_all_of_tag<
            exy::signatures_of<Base>, Tag, _::mp_bind_front<exy::is_nothrow_invocable, Fn>>>;

    struct state : exy::state_base
    {
        EXY_NO_UNIQUE_ADDRESS exy::state_of<Base> _base;
    };

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
            static constexpr exy::state_of<Base>& get_state(exy::state_base& s) noexcept
            {
                return Cont::get_state(s)._base;
            }

            template <exy::signature_with_tag<Tag> S>
            static constexpr exy::continuation continuation_for(exy::state_base& s) noexcept
            {
                _w&              self   = Cont::get_future(s);
                exy::storage_ref result = Cont::get_result_storage(s);

                try
                {
                    auto& [... args] = result.peek<S>();
                    exy_invoke(exy_mov(self)._fn, exy_fwd(args)...);
                    return &Cont::template call<S>;
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

template <typename Tag>
struct with_t
{
    template <
        exy::future                               F, typename S = exy::signatures_of<F>,
        exy::invocable_with_signature_tag<S, Tag> Fn>
    static constexpr auto operator()(F&& f, Fn&& fn) -> _w<F, Tag, std::decay_t<Fn>>
    {
        return {{}, exy_mov(f), exy_fwd(fn)};
    }

    template <exy::movable Fn>
    static constexpr auto operator()(Fn&& fn)
    {
        return exy::make_adaptor_proxy<with_t>(exy_fwd(fn));
    }
};

inline constexpr with_t<exy::value_tag>   with;
inline constexpr with_t<exy::error_tag>   with_error;
inline constexpr with_t<exy::stopped_tag> with_stopped;
} // namespace exy::futures

#endif // EXY_FUTURE_WITH_HPP_INCLUDED

