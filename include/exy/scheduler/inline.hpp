// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SCHEDULER_INLINE_HPP_INCLUDED
#define EXY_SCHEDULER_INLINE_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/scheduler.hpp>

namespace exy::schedulers
{
inline constexpr struct inline_t : exy::scheduler_base
{
    struct _f : exy::future_base
    {
        using signatures = exy::signatures<exy::value_tag()>;

        static consteval auto storage_spec() noexcept
        {
            return exy::storage_spec::get(signatures());
        }

        template <typename Cont>
        struct op
        {
            static constexpr void* start(exy::ctx_base& ctx)
            {
                exy::storage_ref result = Cont::get_result_storage(ctx);
                EXY_TAIL_CALL    exy::set<signatures, Cont, exy::value_tag()>(result)(ctx);
            }
        };
    };

    static constexpr _f schedule() noexcept
    {
        return {};
    }
} inline_;
} // namespace exy::schedulers

#endif // EXY_SCHEDULER_INLINE_HPP_INCLUDED

