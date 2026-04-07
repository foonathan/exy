// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: EUPL-1.2

#ifndef EXY_SUPPORT_STORAGE_HPP_INCLUDED
#define EXY_SUPPORT_STORAGE_HPP_INCLUDED

#include <compare>
#include <exy/support/base.hpp>
#include <exy/support/invoke.hpp>
#include <exy/support/signature.hpp>

namespace exy
{
struct storage_spec
{
    std::size_t size;
    std::size_t alignment;

    template <typename T>
    static consteval storage_spec get() noexcept
    {
        return {sizeof(T), alignof(T)};
    }
    template <typename Tag, typename... T>
    static consteval storage_spec get(std::type_identity<Tag(T...)>) noexcept
    {
        if constexpr (sizeof...(T) == 0)
            return {0, 1};

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
    struct empty
    {};

    alignas(
        Spec.alignment
    ) std::conditional_t<Spec.size == 0, empty, unsigned char[Spec.size]> _buffer;
    friend class storage_ref;
};

class storage_ref
{
public:
    constexpr storage_ref() noexcept : _ptr(nullptr) {}

    template <storage_spec Spec>
    constexpr storage_ref(storage<Spec>& s) noexcept : _ptr(&s._buffer)
    {}

    constexpr void emplace_result(auto&& fn) noexcept(noexcept(auto(exy_fwd(fn)())))
    {
        ::new(_ptr) auto(exy_fwd(fn)());
    }

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
    template <typename T>
    constexpr auto with_raw(auto&& fn)
    {
        auto ptr = static_cast<T*>(_ptr);
        try
        {
            if constexpr (std::is_void_v<exy::invoke_result_t<decltype(fn), T&&>>)
            {
                exy_invoke(exy_fwd(fn), exy_mov(*ptr));
                ptr->~T();
            }
            else
            {
                auto result = exy_invoke(exy_fwd(fn), exy_mov(*ptr));
                ptr->~T();
                return result;
            }
        }
        catch (...)
        {
            ptr->~T();
            throw;
        }
    }

    template <typename S>
    constexpr auto get() noexcept
    {
        return get_raw<exy::signature_arguments_as<S, _::mp_quote<exy::pack>>>();
    }
    template <typename S>
    constexpr auto with(auto&& fn)
    {
        return with_raw<exy::signature_arguments_as<S, _::mp_quote<exy::pack>>>(exy_fwd(fn));
    }

    template <typename T>
    constexpr T& peek_raw() noexcept
    {
        return *static_cast<T*>(_ptr);
    }

    template <typename S>
    constexpr auto& peek() noexcept
    {
        return peek_raw<exy::signature_arguments_as<S, _::mp_quote<exy::pack>>>();
    }

private:
    void* _ptr;
};
} // namespace exy

#endif // EXY_SUPPORT_STORAGE_HPP_INCLUDED

