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

    template <typename Tag, typename... T>
    static consteval storage_spec get(std::type_identity<Tag(T...)>) noexcept
    {
        using pack = exy::pack<T...>;
        return {sizeof(pack), alignof(pack)};
    }
    template <typename... S>
    static consteval storage_spec get(exy::signatures<S...>) noexcept
    {
        return exy::max(get(std::type_identity<S>{})...);
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
    alignas(Spec.alignment) unsigned char _buffer[Spec.size];
    friend class storage_ref;
};

class storage_ref
{
public:
    constexpr storage_ref() noexcept : _ptr(nullptr) {}

    template <storage_spec Spec>
    constexpr storage_ref(storage<Spec>& s) noexcept : _ptr(&s._buffer)
    {}

    template <typename T>
    constexpr void emplace_raw(auto&&... args)
    {
        ::new(_ptr) T(exy_fwd(args)...);
    }

    template <typename S>
    constexpr void emplace(auto&&... args)
    {
        [&]<typename Tag, typename... T>(std::type_identity<Tag(T...)>) {
            emplace_raw<exy::pack<T...>>(exy::make_pack(exy_fwd(args)...));
        }(std::type_identity<S>{});
    }

    template <typename T>
    constexpr T get_raw() noexcept
    {
        auto ptr    = static_cast<T*>(_ptr);
        T    result = exy_mov(*ptr);
        ptr->~T();
        return result;
    }

    template <typename S>
    constexpr decltype(auto) get(auto&& fn)
    {
        return [&]<typename Tag, typename... T>(std::type_identity<Tag(T...)>) -> decltype(auto) {
            using pack = exy::pack<T...>;
            return get_raw<pack>()(exy_fwd(fn));
        }(std::type_identity<S>{});
    }

private:
    void* _ptr;
};
} // namespace exy

#endif // EXY_SUPPORT_STORAGE_HPP_INCLUDED

