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
#include <compare>

#define exy_assert(...) assert(__VA_ARGS__)

#define exy_mov(...)        static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define exy_fwd(...)        static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define exy_invoke(fn, ...) (fn)(__VA_ARGS__)

#define EXY_RETURN(...)                                                                            \
    noexcept(noexcept(__VA_ARGS__))->decltype(__VA_ARGS__)                                         \
    {                                                                                              \
        return __VA_ARGS__;                                                                        \
    }

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

template <typename T>
concept reference = std::is_reference_v<T>;

template <typename Fn, typename... Args>
concept invocable
    = requires (Fn&& fn, Args&&... args) { exy_invoke(exy_fwd(fn), exy_fwd(args)...); };
template <typename Fn, typename... Args>
using invoke_result_t = decltype(exy_invoke(std::declval<Fn>(), std::declval<Args>()...));
} // namespace exy

//=== utility ===//
namespace exy
{
template <auto C>
using constant = std::integral_constant<decltype(C), C>;

constexpr auto max(const auto& h, const auto&... t) noexcept
{
    auto result = h;
    (void)(((t >= h) ? result = h, 0 : 0), ...);
    return result;
}
} // namespace exy

//=== signature ===//
namespace exy
{
template <typename... T>
struct signature_t
{};

struct signature_value_t
{};

template <auto Signature>
using signature_common_value_type
    = decltype([]<typename... T>(signature_t<signature_value_t(T)...>) {
          return std::common_type<T...>{};
      }(Signature))::type;
} // namespace exy

//=== state ===//
namespace exy
{
struct state_base
{};

class state_ref
{
public:
    constexpr state_ref(state_base& state) noexcept : _ptr(&state) {}

    template <auto... MemPtrs>
    constexpr auto& get() const noexcept
    {
        return
            []<typename T, typename C, T C::* Mem, auto... Tail>(
                this auto recurse, state_base* cur, exy::constant<Mem>, exy::constant<Tail>... tail
            ) noexcept -> auto& {
                auto& obj = static_cast<C*>(cur)->*Mem;
                if constexpr (sizeof...(Tail) == 0)
                    return obj;
                else
                    return recurse(&obj, tail...);
            }(_ptr, exy::constant<MemPtrs>{}...);
    }

private:
    state_base* _ptr;
};
} // namespace exy

//=== storage ===//
namespace exy
{
struct storage_spec
{
    std::size_t size;
    std::size_t alignment;

    template <exy::object T>
    static consteval storage_spec get(std::type_identity<T> = {}) noexcept
    {
        return {sizeof(T), alignof(T)};
    }
    template <exy::reference T>
    static consteval storage_spec get(std::type_identity<T> = {}) noexcept
    {
        return {sizeof(void*), alignof(void*)};
    }
    template <std::same_as<void> T>
    static consteval storage_spec get(std::type_identity<T> = {}) noexcept
    {
        return {0, 1};
    }
    template <typename... T>
    static consteval storage_spec get(exy::signature_t<exy::signature_value_t(T)...>) noexcept
    {
        return get(std::type_identity<T>{}...);
    }
    template <typename... T>
    static consteval storage_spec get(T... t) noexcept
    {
        return exy::max(get(t)...);
    }

    friend bool                           operator==(storage_spec lhs, storage_spec rhs) = default;
    friend constexpr std::strong_ordering operator<=>(storage_spec lhs, storage_spec rhs) noexcept
    {
        if (lhs.size != rhs.size)
            return lhs.size <=> rhs.size;
        return lhs.alignment <=> rhs.alignment;
    }
};

template <storage_spec Spec>
class storage
{
public:
    storage()  = default;
    ~storage() = default;

    storage(const storage&)            = delete;
    storage& operator=(const storage&) = delete;

private:
    alignas(Spec.alignment) unsigned char _buffer[Spec.size == 0 ? 1 : Spec.size];
    friend class storage_ref;
};

class storage_ref
{
public:
    template <storage_spec Spec>
    constexpr storage_ref(storage<Spec>& s) noexcept : _ptr(&s._buffer)
    {}

    template <exy::object T>
    constexpr void emplace(
        auto&&... args
    ) noexcept(std::is_nothrow_constructible_v<T, decltype(args)...>)
    {
        ::new(_ptr) T(exy_fwd(args)...);
    }
    template <exy::reference RefT>
    constexpr void emplace(std::type_identity_t<RefT> ref) noexcept
    {
        ::new(_ptr) auto(&ref);
    }

    template <exy::object T>
    constexpr T get() noexcept
    {
        auto ptr    = static_cast<T*>(_ptr);
        auto result = exy_mov(*ptr);
        ptr->~T();
        return result;
    }
    template <exy::reference T>
    constexpr T get() noexcept
    {
        return static_cast<T>(*static_cast<std::remove_cvref_t<T>*>(_ptr));
    }

private:
    void* _ptr;
};
} // namespace exy

//=== future ===//
namespace exy
{
struct future_base
{};

template <typename T>
concept future = std::derived_from<T, future_base>;
} // namespace exy

#endif // EXY_SUPPORT_BASE_HPP_INCLUDED

