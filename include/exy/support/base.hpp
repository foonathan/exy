// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_BASE_HPP_INCLUDED
#define EXY_SUPPORT_BASE_HPP_INCLUDED

// IWYU pragma: begin_exports
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <type_traits>
// IWYU pragma: end_exports

#include <cassert>
#include <boost/mp11.hpp>

#define exy_assert(...) assert(__VA_ARGS__)

#define exy_mov(...) static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define exy_fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define EXY_NO_UNIQUE_ADDRESS [[no_unique_address]]
#define EXY_TAIL_CALL         [[clang::musttail]] return

//=== concepts ===//
namespace exy
{
template <typename T>
concept object = std::is_object_v<T> && std::is_same_v<T, std::remove_cv_t<T>>;

template <typename T>
concept movable_object = exy::object<T> && std::movable<T>;

template <typename T>
concept movable = exy::movable_object<std::remove_cvref_t<T>>;
} // namespace exy

//=== utility ===//
namespace exy
{
namespace _
{
    using namespace boost::mp11;

    template <typename L>
    using mp_is_unit_list = std::bool_constant<mp_size<L>::value == 1>;

    template <typename L>
        requires mp_is_unit_list<L>::value
    using mp_only = mp_front<L>;
} // namespace _

template <auto C>
using constant = std::integral_constant<decltype(C), C>;

constexpr auto max(const auto& h, const auto&... t) noexcept
{
    auto result = h;
    (void)(((t >= h) ? result = t, 0 : 0), ...);
    return result;
}

constexpr auto make_pack(exy::movable auto&&... args) noexcept
{
    return [... elements = exy_fwd(args)](auto&& fn) mutable -> decltype(auto) {
        return exy_fwd(fn)(exy_mov(elements)...);
    };
}

template <exy::movable_object... Ts>
using pack = decltype(make_pack(std::declval<Ts>()...));
} // namespace exy

#endif // EXY_SUPPORT_BASE_HPP_INCLUDED

