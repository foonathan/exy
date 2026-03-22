// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SCHEDULER_DEFER_HPP_INCLUDED
#define EXY_SCHEDULER_DEFER_HPP_INCLUDED

#include <mutex>
#include <condition_variable>
#include <exy/support/future.hpp>
#include <exy/support/scheduler.hpp>

namespace exy::schedulers
{
class defer;
}

namespace exy
{
class run_loop
{
    struct job
    {
        exy::continuation continuation;
        exy::state_base*  s;
        job*              next;
    };

    enum class state : unsigned char
    {
        starting,
        running,
        finishing,
    };

public:
    run_loop()                           = default;
    run_loop(const run_loop&)            = delete;
    run_loop& operator=(const run_loop&) = delete;

    void run() noexcept
    {
        {
            std::lock_guard lock(_mutex);
            switch (_state)
            {
            case state::starting:
                _state = state::running;
                break;
            case state::running:
                exy_assert(false);
                break;
            case state::finishing:
                break;
            }
        }

        while (auto j = pop())
            j->continuation(*j->s);
    }

    void finish() noexcept
    {
        std::lock_guard lock(_mutex);
        _state = state::finishing;
        _cv.notify_one();
    }

private:
    void push(job* j) noexcept
    {
        std::lock_guard lock(_mutex);
        if (_tail != nullptr)
        {
            _tail->next = j;
            _tail       = j;
        }
        else
        {
            _head = _tail = j;
            _cv.notify_one();
        }
    }

    job* pop() noexcept
    {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [&] { return _head != nullptr || _state == state::finishing; });
        if (_head == nullptr)
            return nullptr;

        auto result = std::exchange(_head, _head->next);
        if (_head == nullptr)
            _tail = nullptr;
        return result;
    }

    // TODO: lock free
    std::mutex              _mutex;
    std::condition_variable _cv;
    job*                    _head  = nullptr;
    job*                    _tail  = nullptr;
    state                   _state = state::starting;

    friend schedulers::defer;
};
} // namespace exy

namespace exy::schedulers
{
class defer : public exy::scheduler_base
{
    struct _f : exy::future_base
    {
        exy::run_loop* _loop;

        using signatures = exy::signatures<exy::value_tag()>;

        struct state : exy::state_base
        {
            exy::run_loop::job _job;

            constexpr state(_f&) noexcept : _job{} {}
        };

        static consteval auto storage_spec() noexcept
        {
            return exy::storage_spec::get(signatures());
        }

        template <typename Cont>
        struct op
        {
            static constexpr void* start(exy::state_base& s)
            {
                _f&              self   = Cont::get_future(s);
                state&           state  = Cont::get_state(s);
                exy::storage_ref result = Cont::get_result_storage(s);

                state._job = {
                    .continuation = exy::set<signatures, Cont, exy::value_tag()>(result),
                    .s            = &s,
                };
                self._loop->push(&state._job);
                return nullptr;
            }
        };
    };

public:
    explicit defer(exy::run_loop& loop) noexcept : _loop(&loop) {}

    constexpr _f schedule() const noexcept
    {
        return {{}, _loop};
    }

private:
    exy::run_loop* _loop;
};
} // namespace exy::schedulers

#endif // EXY_SCHEDULER_DEFER_HPP_INCLUDED

