// Copyright (C) 2026 Jonathan Müller and exy contributors
// SPDX-License-Identifier: BSL-1.0

#ifndef EXY_SUPPORT_SIGNATURE_HPP_INCLUDED
#define EXY_SUPPORT_SIGNATURE_HPP_INCLUDED

#include <exy/support/base.hpp>

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
using signatures_transform_tag = _::mp_unique<_::mp_transform_if_q<
    _::mp_quote<TagFrom::template is>,
    _::mp_compose<exy::signature_argument, QFn::template fn, TagTo::template make>, S>>;

template <typename S, typename Tag, typename QPredicate>
constexpr bool signatures_all_of_tag = _::mp_all_of<
    signatures_fold_tag<
        S, Tag, _::mp_quote<_::mp_list>,
        _::mp_compose_q<_::mp_quote<exy::signature_argument>, QPredicate>>,
    _::mp_identity_t>::value;

template <typename S, bool Noexcept>
using signatures_insert_exception
    = std::conditional_t<Noexcept, S, _::mp_set_push_back<S, exy::error_tag(std::exception_ptr)>>;
} // namespace exy

#endif // EXY_SUPPORT_SIGNATURE_HPP_INCLUDED

