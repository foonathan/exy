// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <any>
#include <memory>
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

    explicit test_result(auto tag, auto&&... args)
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
      })
    {}

    friend std::ostream& operator<<(std::ostream& os, const test_result& result)
    {
        return os << result.fn_to_string(result.args);
    }
};

template <typename Tag, typename... Args>
struct test_result_matcher : Catch::Matchers::MatcherGenericBase
{
    std::tuple<Args...> expected_args;

    test_result_matcher(Tag, Args&&... args) : expected_args(exy_mov(args)...) {}

    bool match(const test_result& result) const
    {
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
        return exy_mov(ss).str();
    }
};

inline constexpr struct test_run_t
{
    template <typename F>
    struct _state : exy::state_base
    {
        exy::state_of<F> _s;

        constexpr explicit _state(
            F&& f
        ) noexcept(std::is_nothrow_constructible_v<exy::state_of<F>, F&&>)
        : _s(exy_mov(f))
        {}
    };

    template <typename F>
    static consteval auto _storage_spec() noexcept
    {
        return exy::max(
            F::storage_spec(), exy::storage_spec{sizeof(test_result), alignof(test_result)}
        );
    }

    struct _c
    {
        template <typename S>
        static constexpr void* call(exy::state_ref, exy::storage_ref result)
        {
            result.get<S>([&](auto&&... args) {
                result.emplace_raw<test_result>(exy::signature_tag<S>{}, exy_fwd(args)...);
            });
            return nullptr;
        }
    };

    template <exy::future F>
    static constexpr test_result operator()(F&& f)
    {
        _state<F>                        state(exy_mov(f));
        exy::storage<_storage_spec<F>()> result;
        F::template op<_c, &_state<F>::_s>::start(state, result);
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

