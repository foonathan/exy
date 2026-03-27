// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SCHEDULER_PARALLEL_HPP_INCLUDED
#define EXY_SCHEDULER_PARALLEL_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/scheduler.hpp>

namespace exy
{
class parallel_scheduler_backend
{
public:
    struct job
    {
        exy::continuation continuation;
        /// This can be used for an intrinsically linked list of pending jobs.
        job* next;
    };

    /// The job reference is valid until the continuation is called.
    constexpr virtual void schedule(job& j, exy::ctx_base& ctx) const = 0;

protected:
    parallel_scheduler_backend()          = default;
    virtual ~parallel_scheduler_backend() = default;
};
} // namespace exy

namespace exy::schedulers
{
class parallel : public exy::scheduler_base
{
    struct _f : exy::future_base
    {
        const parallel_scheduler_backend* _backend;

        using signatures = exy::signatures<exy::value_tag(), exy::error_tag(std::exception_ptr)>;

        static consteval auto storage_spec() noexcept
        {
            return exy::storage_spec::get(signatures());
        }

        template <typename Cont>
        struct op
        {
            parallel_scheduler_backend::job _job;

            static constexpr void* start(exy::ctx_base& ctx)
            {
                op&              self   = Cont::get_op(ctx);
                _f&              f      = Cont::get_future(ctx);
                exy::storage_ref result = Cont::get_result_storage(ctx);

                auto cont = [&] -> exy::continuation {
                    try
                    {
                        self._job = {
                            .next         = nullptr,
                            .continuation = exy::set<signatures, Cont, exy::value_tag()>(result),
                        };
                        f._backend->schedule(self._job, ctx);
                        return nullptr;
                    }
                    catch (...)
                    {
                        return exy::set_exception<signatures, Cont>(result);
                    }
                }();
                if (cont)
                    EXY_TAIL_CALL cont(ctx);
                else
                    return nullptr;
            }
        };
    };

public:
    constexpr explicit parallel(const parallel_scheduler_backend& backend) noexcept
    : _backend(&backend)
    {}

    constexpr _f schedule() const noexcept
    {
        return {{}, _backend};
    }

private:
    const parallel_scheduler_backend* _backend;
};
} // namespace exy::schedulers

#endif // EXY_SCHEDULER_PARALLEL_HPP_INCLUDED

