// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <any>
#include <atomic>
#include <memory>
#include <thread>
#include <tuple>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <exy/support/future.hpp>

struct test_result
{
    template <typename Tag, typename... Args>
    static std::string to_string(Tag, const Args&... args)
    {
        std::ostringstream os;
        if constexpr (std::same_as<Tag, exy::value_tag>)
            os << "value";
        else if constexpr (std::same_as<Tag, exy::error_tag>)
            os << "error";
        else if constexpr (std::same_as<Tag, exy::stopped_tag>)
            os << "stopped";
        else
            os << "???";

        os << "(";
        auto first = true;
        ((os << (first ? "" : ", ") << Catch::StringMaker<Args>::convert(args), first = false),
         ...);
        os << ")";

        return exy_mov(os).str();
    }

    std::any tag;
    std::any args;
    std::string (*fn_to_string)(const std::any&);
    std::thread::id completion_id;

    explicit test_result(std::thread::id completion_id, auto tag, auto&&... args)
    : tag(tag), args(
                    std::make_shared<std::tuple<std::decay_t<decltype(args)>...>>(
                        std::make_tuple(exy_fwd(args)...)
                    )
                ),
      fn_to_string([](const std::any& a) {
          const auto& tuple
              = *std::any_cast<std::shared_ptr<std::tuple<std::decay_t<decltype(args)>...>>>(a);
          return std::apply(
              [&](const auto&... unpacked_args) {
                  return to_string(decltype(tag){}, unpacked_args...);
              },
              tuple
          );
      }),
      completion_id(completion_id)
    {}

    friend std::ostream& operator<<(std::ostream& os, const test_result& result)
    {
        return os << result.fn_to_string(result.args) << " on "
                  << (result.completion_id == std::this_thread::get_id() ? "main thread"
                                                                         : "background thread");
    }
};

inline constexpr struct thread_change_t
{
} thread_change;
inline constexpr struct no_thread_change_t
{
} no_thread_change;

template <typename Tag, typename... Args>
struct test_result_matcher : Catch::Matchers::MatcherGenericBase
{
    std::tuple<Args...> expected_args;
    std::optional<bool> expected_thread_change;

    test_result_matcher(Tag, Args&&... args) : expected_args(exy_mov(args)...) {}
    test_result_matcher(thread_change_t, Tag, Args&&... args)
    : expected_args(exy_mov(args)...), expected_thread_change(true)
    {}
    test_result_matcher(no_thread_change_t, Tag, Args&&... args)
    : expected_args(exy_mov(args)...), expected_thread_change(false)
    {}

    bool match(const test_result& result) const
    {
        if (expected_thread_change)
        {
            if ((result.completion_id != std::this_thread::get_id()) != *expected_thread_change)
                return false;
        }

        if (result.tag.type() != typeid(Tag))
            return false;

        auto actual_args = std::any_cast<std::shared_ptr<std::tuple<Args...>>>(&result.args);
        return actual_args && expected_args == **actual_args;
    }

    std::string describe() const override
    {
        std::ostringstream ss;
        ss << "== ";
        std::apply(
            [&](const auto&... unpacked_args) {
                ss << test_result::to_string(Tag{}, unpacked_args...);
            },
            expected_args
        );
        ss << " on ";
        if (expected_thread_change)
        {
            ss << (*expected_thread_change ? "background thread" : "main thread");
        }
        else
        {
            ss << "any thread";
        }
        return exy_mov(ss).str();
    }
};

inline constexpr struct test_run_t
{
    template <typename F>
    struct _state : exy::state_base
    {
        exy::state_of<F>  _s;
        std::atomic<bool> _done = false;

        constexpr explicit _state(F& f) noexcept(exy::has_nothrow_constructible_state<F>) : _s(f) {}
    };

    template <typename F>
    static consteval auto _storage_spec() noexcept
    {
        return exy::max(
            F::storage_spec(), exy::storage_spec{sizeof(test_result), alignof(test_result)}
        );
    }

    template <typename F>
    struct _c
    {
        static constexpr F& get(exy::future_base& f, exy::state_base&) noexcept
        {
            return static_cast<F&>(f);
        }
        static constexpr exy::state_of<F>& get(exy::state_base& s) noexcept
        {
            return static_cast<_state<F>&>(s)._s;
        }

        template <typename S>
        static constexpr void* call(exy::future_base&, exy::state_base& s, exy::storage_ref result)
        {
            auto& self = static_cast<_state<F>&>(s);

            auto [... args] = result.get<S>();
            result.emplace_raw<test_result>(
                std::this_thread::get_id(), exy::signature_tag<S>{}, exy_mov(args)...
            );

            self._done.store(true, std::memory_order_release);
            self._done.notify_one();

            return nullptr;
        }
    };

    template <exy::future F>
    static constexpr test_result operator()(F&& f)
    {
        _state<F> state(f);

        exy::storage<_storage_spec<F>()> result;
        F::template op<_c<F>>::start(f, state, result);

        state._done.wait(false, std::memory_order_acquire);

        return exy::storage_ref(result).get_raw<test_result>();
    }
} test_run;

#define REQUIRE_FUTURE(expr, ...)                                                                  \
    REQUIRE_THAT(test_run(auto(expr)), test_result_matcher(__VA_ARGS__))

#define REQUIRE_SIGNATURES(expr, ...)                                                              \
    static_assert(std::same_as<exy::signatures_of<decltype(expr)>, exy::signatures<__VA_ARGS__>>)

struct move_only
{
    int value;

    explicit move_only(int value) : value(value) {}
    move_only(move_only&&) noexcept(false)            = default;
    move_only& operator=(move_only&&) noexcept(false) = default;
    ~move_only()                                      = default;
    move_only(const move_only&)                       = delete;
    move_only& operator=(const move_only&)            = delete;

    bool operator==(const move_only&) const = default;

    [[maybe_unused]] friend std::ostream& operator<<(std::ostream& os, const move_only& m)
    {
        return os << "move_only(" << m.value << ")";
    }
};

template <typename... S, exy::future F>
constexpr auto with_signature(F&& f)
{
    struct F2 : F
    {
        using signatures = exy::_::mp_set_push_back<exy::signatures_of<F>, S...>;
    };
    return F2{exy_mov(f)};
}

