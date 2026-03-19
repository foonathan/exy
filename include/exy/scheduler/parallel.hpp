// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SCHEDULER_PARALLEL_HPP_INCLUDED
#define EXY_SCHEDULER_PARALLEL_HPP_INCLUDED

#include <exy/support/future.hpp>
#include <exy/support/scheduler.hpp>

namespace exy::schedulers
{
class parallel_backend
{
public:
    struct job
    {
        exy::continuation continuation;
        /// This can be used for an intrinsically linked list of pending jobs.
        job* next;
    };

    /// The job reference is valid until the continuation is called.
    constexpr virtual void schedule(job& j, exy::state_base& s, exy::storage_ref result) const = 0;

protected:
    parallel_backend()          = default;
    virtual ~parallel_backend() = default;
};

class parallel : public exy::scheduler_base
{
    struct _f : exy::future_base
    {
        const parallel_backend* _backend;

        using signatures = exy::signatures<exy::value_tag(), exy::error_tag(std::exception_ptr)>;

        struct state : exy::state_base
        {
            parallel_backend::job _job;

            constexpr state(_f&) noexcept : _job{.continuation = nullptr, .next = nullptr} {}
        };

        static consteval auto storage_spec() noexcept
        {
            return exy::storage_spec::get(signatures());
        }

        template <typename Cont>
        struct op
        {
            static constexpr void* start(exy::state_base& s, exy::storage_ref result)
            {
                _f&    self  = Cont::get_future(s);
                state& state = Cont::get_state(s);

                auto cont = [&] -> exy::continuation {
                    try
                    {
                        state._job = {
                            .next         = nullptr,
                            .continuation = exy::set<signatures, Cont, exy::value_tag()>(result),
                        };
                        self._backend->schedule(state._job, s, result);
                        return nullptr;
                    }
                    catch (...)
                    {
                        return exy::set_exception<signatures, Cont>(result);
                    }
                }();
                if (cont)
                    EXY_TAIL_CALL cont(s, result);
                else
                    return nullptr;
            }
        };
    };

public:
    constexpr explicit parallel(const parallel_backend& backend) noexcept : _backend(&backend) {}

    constexpr _f schedule() const noexcept
    {
        return {{}, _backend};
    }

private:
    const parallel_backend* _backend;
};
} // namespace exy::schedulers

#endif // EXY_SCHEDULER_PARALLEL_HPP_INCLUDED

