// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_STORAGE_HPP_INCLUDED
#define EXY_SUPPORT_STORAGE_HPP_INCLUDED

#include <compare>
#include <exy/support/base.hpp>
#include <exy/support/signature.hpp>

namespace exy
{
struct storage_spec
{
    std::size_t size;
    std::size_t alignment;

    template <exy::object T>
    static consteval storage_spec get(std::type_identity<T>) noexcept
    {
        return {sizeof(T), alignof(T)};
    }
    template <typename... Tag, typename... T>
    static consteval storage_spec get(exy::signatures<Tag(T)...>) noexcept
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

    template <exy::object T>
    constexpr T get() noexcept
    {
        auto ptr    = static_cast<T*>(_ptr);
        auto result = exy_mov(*ptr);
        ptr->~T();
        return result;
    }

private:
    void* _ptr;
};
} // namespace exy

#endif // EXY_SUPPORT_STORAGE_HPP_INCLUDED

