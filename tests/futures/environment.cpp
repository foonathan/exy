// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#include <exy/future/environment.hpp>

#include "test.hpp"

namespace exyf = exy::futures;

namespace
{
constexpr struct query_int_t : exy::query_base
{
    using result_type = int;
} query_int;

constexpr struct query_int_default_t : exy::query_base
{
    using result_type = int;

    static constexpr int default_value() noexcept
    {
        return -1;
    }
} query_int_default;

constexpr struct query_move_only_t : exy::query_base
{
    using result_type = move_only;
} query_move_only;

constexpr struct query_untyped_t : exy::query_base
{
} query_untyped;

constexpr struct env_without_default_t : test_env_t
{
    using test_env_t::query;

    static constexpr auto query(query_int_t) noexcept
    {
        return 11;
    }

    static constexpr auto query(query_move_only_t) noexcept
    {
        return move_only(17);
    }

    static constexpr auto query(query_untyped_t) noexcept
    {
        return std::string("hello");
    }
} env_without_default;

constexpr struct env_with_default_t : env_without_default_t
{
    using env_without_default_t::query;

    static constexpr auto query(query_int_default_t) noexcept
    {
        return 42;
    }
} env_with_default;
} // namespace

TEST_CASE("read_env in env_without_default", "[futures]")
{
    constexpr auto& test_env = env_without_default;

    auto read_int = exyf::read_env(query_int);
    REQUIRE_SIGNATURES(read_int, exy::value_tag(int));
    REQUIRE_FUTURE(read_int, exy::value_tag(), 11);

    auto read_int_default = exyf::read_env(query_int_default);
    REQUIRE_SIGNATURES(read_int_default, exy::value_tag(int));
    REQUIRE_FUTURE(read_int_default, exy::value_tag(), -1);

    auto read_move_only = exyf::read_env(query_move_only);
    REQUIRE_SIGNATURES(
        read_move_only, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    REQUIRE_FUTURE(read_move_only, exy::value_tag(), move_only(17));
}

TEST_CASE("read_env in env_with_default", "[futures]")
{
    constexpr auto& test_env = env_with_default;

    auto read_int = exyf::read_env(query_int);
    REQUIRE_SIGNATURES(read_int, exy::value_tag(int));
    REQUIRE_FUTURE(read_int, exy::value_tag(), 11);

    auto read_int_default = exyf::read_env(query_int_default);
    REQUIRE_SIGNATURES(read_int_default, exy::value_tag(int));
    REQUIRE_FUTURE(read_int_default, exy::value_tag(), 42);

    auto read_move_only = exyf::read_env(query_move_only);
    REQUIRE_SIGNATURES(
        read_move_only, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    REQUIRE_FUTURE(read_move_only, exy::value_tag(), move_only(17));
}

TEST_CASE("with_env in env_without_default", "[futures]")
{
    constexpr auto& test_env = env_without_default;

    auto with = exyf::with_env<int>(
        [](int a, std::string_view b) { return a + int(b.size()); }, query_int_default,
        query_untyped
    );
    REQUIRE_SIGNATURES(with, exy::value_tag(int), exy::error_tag(std::exception_ptr));
    REQUIRE_FUTURE(with, exy::value_tag(), -1 + 5);

    auto with_nothrow = exyf::with_env<int, true>(
        [](int a, std::string_view b) { return a + int(b.size()); }, query_int_default,
        query_untyped
    );
    REQUIRE_SIGNATURES(with_nothrow, exy::value_tag(int));
    REQUIRE_FUTURE(with_nothrow, exy::value_tag(), -1 + 5);

    auto with_move_only = exyf::with_env<move_only, true>(std::identity{}, query_move_only);
    REQUIRE_SIGNATURES(
        with_move_only, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    REQUIRE_FUTURE(with_move_only, exy::value_tag(), move_only(17));
}

TEST_CASE("with_env in env_with_default", "[futures]")
{
    constexpr auto& test_env = env_with_default;

    auto with = exyf::with_env<int>(
        [](int a, std::string_view b) { return a + int(b.size()); }, query_int_default,
        query_untyped
    );
    REQUIRE_SIGNATURES(with, exy::value_tag(int), exy::error_tag(std::exception_ptr));
    REQUIRE_FUTURE(with, exy::value_tag(), 42 + 5);

    auto with_nothrow = exyf::with_env<int, true>(
        [](int a, std::string_view b) { return a + int(b.size()); }, query_int_default,
        query_untyped
    );
    REQUIRE_SIGNATURES(with_nothrow, exy::value_tag(int));
    REQUIRE_FUTURE(with_nothrow, exy::value_tag(), 42 + 5);

    auto with_move_only = exyf::with_env<move_only, true>(std::identity{}, query_move_only);
    REQUIRE_SIGNATURES(
        with_move_only, exy::value_tag(move_only), exy::error_tag(std::exception_ptr)
    );
    REQUIRE_FUTURE(with_move_only, exy::value_tag(), move_only(17));
}

