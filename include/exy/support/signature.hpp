// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_SIGNATURE_HPP_INCLUDED
#define EXY_SUPPORT_SIGNATURE_HPP_INCLUDED

#include <exy/support/base.hpp>

namespace exy
{
template <typename T>
struct _signature_trait;
template <typename Tag, typename... T>
struct _signature_trait<Tag(T...)>
{
    using tag       = Tag;
    using arguments = _::mp_list<T...>;
};

template <typename T>
using signature_tag = typename _signature_trait<T>::tag;

template <typename T>
using signature_arguments = typename _signature_trait<T>::arguments;
template <typename T, typename Q>
using signature_arguments_as = _::mp_apply_q<Q, signature_arguments<T>>;

template <typename Signature, typename Tag>
concept signature_with_tag = std::same_as<signature_tag<Signature>, Tag>;
} // namespace exy

namespace exy
{
template <typename Tag, typename... T>
using _make_signature = Tag(T...);
template <typename Tag, typename... T>
using make_signature = _::mp_eval_if_c<
    std::same_as<_::mp_list<T...>, _::mp_list<void>>, Tag(), _make_signature, Tag, T...>;

struct value_tag
{
    template <typename... T>
    using make = make_signature<value_tag, T...>;

    template <typename T>
    using is = std::bool_constant<signature_with_tag<T, value_tag>>;
};
struct error_tag
{
    template <typename... T>
    using make = make_signature<error_tag, T...>;

    template <typename T>
    using is = std::bool_constant<signature_with_tag<T, error_tag>>;
};
struct stopped_tag
{
    template <typename... T>
    using make = make_signature<stopped_tag, T...>;

    template <typename T>
    using is = std::bool_constant<signature_with_tag<T, stopped_tag>>;
};
} // namespace exy

namespace exy
{

template <typename... T>
struct signatures
{};

template <typename S, typename Tag, typename QSum, typename QProduct>
using signatures_fold_tag = _::mp_apply_q<
    QSum,
    _::mp_transform_q<
        _::mp_bind_back<exy::signature_arguments_as, QProduct>, _::mp_filter<Tag::template is, S>>>;

template <typename S, typename TagFrom, typename QFn, typename TagTo>
using signatures_transform_tag = _::mp_unique<_::mp_transform_if_q<
    _::mp_quote<TagFrom::template is>,
    _::mp_compose<
        _::mp_bind_back<exy::signature_arguments_as, QFn>::template fn, TagTo::template make>,
    S>>;

template <typename S, typename Tag, typename OtherS>
using signatures_replace_tag
    = _::mp_unique<_::mp_append<_::mp_remove_if<S, Tag::template is>, OtherS>>;

template <typename S, typename Tag, typename QPredicate>
constexpr bool signatures_all_of_tag = _::mp_all_of_q<
    _::mp_filter<Tag::template is, S>,
    _::mp_bind_back<exy::signature_arguments_as, QPredicate>>::value;

template <typename S, typename Tag, typename QPredicate = _::mp_constant_fn<std::true_type>>
constexpr bool signatures_any_of_tag = _::mp_any_of_q<
    _::mp_filter<Tag::template is, S>,
    _::mp_bind_back<exy::signature_arguments_as, QPredicate>>::value;

template <typename S, typename Tag, typename QPredicate = _::mp_constant_fn<std::true_type>>
constexpr bool signatures_none_of_tag = _::mp_none_of_q<
    _::mp_filter<Tag::template is, S>,
    _::mp_bind_back<exy::signature_arguments_as, QPredicate>>::value;

template <typename S, bool Noexcept>
using signatures_insert_exception
    = std::conditional_t<Noexcept, S, _::mp_set_push_back<S, exy::error_tag(std::exception_ptr)>>;
} // namespace exy

#endif // EXY_SUPPORT_SIGNATURE_HPP_INCLUDED

