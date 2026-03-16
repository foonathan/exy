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
#include <boost/mp11.hpp>

#define exy_assert(...) assert(__VA_ARGS__)

#define exy_mov(...)        static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define exy_fwd(...)        static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define exy_invoke(fn, ...) (fn)(__VA_ARGS__)

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
using is_invocable = std::bool_constant<invocable<Fn, Args...>>;

template <typename Fn, typename... Args>
concept nothrow_invocable = invocable<Fn, Args...> && requires (Fn&& fn, Args&&... args) {
    { exy_invoke(exy_fwd(fn), exy_fwd(args)...) } noexcept;
};
template <typename Fn, typename... Args>
using is_nothrow_invocable = std::bool_constant<nothrow_invocable<Fn, Args...>>;

template <typename Fn, typename... Args>
using invoke_result_t = decltype(exy_invoke(std::declval<Fn>(), std::declval<Args>()...));
} // namespace exy

//=== utility ===//
namespace exy
{
namespace _
{
    using namespace boost::mp11;
}

template <auto C>
using constant = std::integral_constant<decltype(C), C>;

constexpr auto max(const auto& h, const auto&... t) noexcept
{
    auto result = h;
    (void)(((t >= h) ? result = t, 0 : 0), ...);
    return result;
}
} // namespace exy

//=== signature ===//
namespace exy
{
template <typename T>
struct _signature_trait;
template <typename Tag, typename T>
struct _signature_trait<Tag(T)>
{
    using tag      = Tag;
    using argument = T;
};

template <typename T>
using signature_tag = typename _signature_trait<T>::tag;
template <typename T>
using signature_argument = typename _signature_trait<T>::argument;

template <typename Signature, typename Tag>
concept signature_with_tag = std::same_as<signature_tag<Signature>, Tag>;

struct value_tag
{
    template <typename... Args>
    using make = value_tag(Args...);

    template <typename T>
    using is = std::bool_constant<signature_with_tag<T, value_tag>>;
};
struct error_tag
{
    template <typename... Args>
    using make = error_tag(Args...);

    template <typename T>
    using is = std::bool_constant<signature_with_tag<T, error_tag>>;
};

template <typename... T>
struct signatures
{};

template <typename S, typename Tag, typename QSum, typename QProduct>
using signatures_fold_tag
    = _::mp_apply_q<QSum, _::mp_transform_q<QProduct, _::mp_filter<Tag::template is, S>>>;

template <typename S, typename TagFrom, typename QFn, typename TagTo>
using signatures_transform_tag = _::mp_transform_if_q<
    _::mp_quote<TagFrom::template is>,
    _::mp_compose<exy::signature_argument, QFn::template fn, TagTo::template make>, S>;

template <typename S, typename Tag, typename QPredicate>
constexpr bool signatures_all_of_tag = _::mp_all_of<
    signatures_fold_tag<
        S, Tag, _::mp_quote<_::mp_list>,
        _::mp_compose_q<_::mp_quote<exy::signature_argument>, QPredicate>>,
    _::mp_identity_t>::value;

template <typename S, bool Noexcept>
using signatures_insert_exception
    = std::conditional_t<Noexcept, S, _::mp_push_back<S, exy::error_tag(std::exception_ptr)>>;
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
    static consteval storage_spec get(std::type_identity<T>) noexcept
    {
        return {sizeof(T), alignof(T)};
    }
    template <exy::reference T>
    static consteval storage_spec get(std::type_identity<T>) noexcept
    {
        return {sizeof(void*), alignof(void*)};
    }
    template <std::same_as<void> T>
    static consteval storage_spec get(std::type_identity<T>) noexcept
    {
        return {0, 1};
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

template <typename F>
using signatures_of = typename F::signatures;

using continuation = void* (*)(exy::state_ref, exy::storage_ref);

template <typename Cont, typename S>
constexpr auto set(exy::storage_ref result, auto&&... args) noexcept(
    std::is_nothrow_constructible_v<exy::signature_argument<S>, decltype(args)...>
) -> continuation
{
    result.emplace<exy::signature_argument<S>>(exy_fwd(args)...);
    return &Cont::template call<S>;
}

template <typename Cont>
constexpr auto set_exception(exy::storage_ref result) noexcept -> continuation
{
    return exy::set<Cont, exy::error_tag(std::exception_ptr)>(result, std::current_exception());
}

template <typename Cont, typename S>
constexpr auto set_or_exception(exy::storage_ref result, auto fn) noexcept -> continuation
{
    using type = exy::signature_argument<S>;
    if constexpr (std::is_nothrow_move_constructible_v<type> && noexcept(fn()))
    {
        return exy::set<Cont, S>(result, fn());
    }
    else
    {
        try
        {
            return exy::set<Cont, S>(result, fn());
        }
        catch (...)
        {
            return exy::set_exception<Cont>(result);
        }
    }
}

template <typename Derived, typename Cont>
struct adapter_continuation
{
    template <typename S>
    static constexpr void* call(exy::state_ref s, exy::storage_ref result)
        requires requires { Derived::template continuation_for<S>(s, result); }
    {
        EXY_TAIL_CALL Derived::template continuation_for<S>(s, result)(s, result);
    }

    template <typename S>
    static constexpr void* call(exy::state_ref s, exy::storage_ref result)
    {
        EXY_TAIL_CALL Cont::template call<S>(s, result);
    }
};
} // namespace exy

#endif // EXY_SUPPORT_BASE_HPP_INCLUDED

