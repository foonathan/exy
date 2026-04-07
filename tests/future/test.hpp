// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#include <any>
#include <atomic>
#include <memory>
#include <thread>
#include <tuple>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <exy/scheduler/parallel.hpp>
#include <exy/support/adapter.hpp>
#include <exy/support/future.hpp>
#include <exy/support/query.hpp>
#include <exy/sync_wait.hpp>

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
    template <typename Env, typename Base>
    struct _f : exy::future_base
    {
        const Env& _env;
        Base       _base;

        using signatures = exy::signatures<exy::value_tag(test_result)>;

        static consteval auto storage_spec() noexcept
        {
            return exy::max(Base::storage_spec(), exy::storage_spec::get(signatures()));
        }

        template <typename Cont>
        struct op
        {
            struct _c : exy::adapter_continuation<_c, Cont>
            {
                static constexpr Base& get_future(exy::ctx_base& ctx) noexcept
                {
                    return Cont::get_future(ctx)._base;
                }
                static constexpr exy::op_of<Base, _c>& get_op(exy::ctx_base& ctx) noexcept
                {
                    return Cont::get_op(ctx)._base;
                }

                static constexpr auto query(exy::query auto q, exy::ctx_base& ctx) noexcept
                    -> decltype(std::declval<const Env&>().query(q))
                {
                    return Cont::get_future(ctx)._env.query(q);
                }

                template <typename S>
                static constexpr exy::continuation continuation_for(exy::ctx_base& ctx) noexcept
                {
                    exy::storage_ref result = Cont::get_result_storage(ctx);

                    auto [... args] = result.get<S>();
                    return exy::set<signatures, Cont, exy::value_tag(test_result)>(
                        result,
                        test_result(
                            std::this_thread::get_id(), exy::signature_tag<S>{}, exy_mov(args)...
                        )
                    );
                }
            };

            exy::op_of<Base, _c> _base;

            static constexpr void* start(exy::ctx_base& ctx)
            {
                op&           self = Cont::get_op(ctx);
                EXY_TAIL_CALL self._base.start(ctx);
            }
        };
    };

    template <typename Env, exy::future F>
    static constexpr test_result operator()(const Env& env, F&& f)
    {
        auto [result] = *exy::sync_wait(_f{{}, env, exy_mov(f)});
        return result;
    }
} test_run;

inline constexpr struct test_env_t
{
    test_env_t()                             = default;
    test_env_t(const test_env_t&)            = delete;
    test_env_t& operator=(const test_env_t&) = delete;

    static constexpr auto query(exy::query auto) noexcept
    {
        return exy::no_such_query{};
    }
} test_env;

#define CHECK_FUTURE(expr, ...)                                                                    \
    CHECK_THAT(test_run(test_env, auto(expr)), test_result_matcher(__VA_ARGS__))

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

constexpr struct background_backend : exy::parallel_scheduler_backend
{
    void schedule(job& j, exy::ctx_base& ctx) const override
    {
        // For the test we don't really care whether it truly runs in the background, just as long
        // as it is a different thread.
        std::jthread([&] { j.continuation(ctx); });
    }
} background_backend;

const struct failing_backend : exy::parallel_scheduler_backend
{
    std::exception_ptr ex = std::make_exception_ptr(0);

    void schedule(job&, exy::ctx_base&) const override
    {
        std::rethrow_exception(ex);
    }
} failing_backend;

