// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The few pieces of `<utility>` and `<type_traits>` that Utl needs, as
 *        avr-gcc comes without a C++ standard library; not part of the API.
 */

#ifndef UTL_SRC_UTL_INTERNAL_TYPE_TRAITS_H_
#define UTL_SRC_UTL_INTERNAL_TYPE_TRAITS_H_

namespace utl {
namespace internal {

// std::declval. Declared only: it names a value of type T in decltype, where
// nothing is evaluated.
template <typename T>
T&& DeclVal();

// Declared only, like DeclVal: a call compiles if its argument converts to To
// implicitly.
template <typename To>
void ConvertTo(To);

// std::void_t: void, if every type in Ts is valid. Through a struct, as an
// alias template alone may ignore arguments it does not use (CWG 1558).
template <typename... Ts>
struct VoidImpl {
  using Type = void;
};
template <typename... Ts>
using Void = typename VoidImpl<Ts...>::Type;

// void, if a T is initialized from each of Ts in braces, which refuse
// narrowing. An alias of its own, as cppcheck (2.19 to 2.22) reads the pack
// expansion `internal::Void<decltype(...)...>>` in a default template argument
// as a syntax error.
template <typename T, typename... Ts>
using BracedFrom = Void<decltype(T{DeclVal<Ts>()})...>;

// std::remove_cv_t.
template <typename T>
struct RemoveCvImpl {
  using Type = T;
};
template <typename T>
struct RemoveCvImpl<T const> {
  using Type = T;
};
template <typename T>
struct RemoveCvImpl<T volatile> {
  using Type = T;
};
template <typename T>
struct RemoveCvImpl<T const volatile> {
  using Type = T;
};
template <typename T>
using RemoveCv = typename RemoveCvImpl<T>::Type;

// std::remove_reference_t.
template <typename T>
struct RemoveReferenceImpl {
  using Type = T;
};
template <typename T>
struct RemoveReferenceImpl<T&> {
  using Type = T;
};
template <typename T>
struct RemoveReferenceImpl<T&&> {
  using Type = T;
};
template <typename T>
using RemoveReference = typename RemoveReferenceImpl<T>::Type;

// std::remove_cvref_t, from C++20: the type a function returns, whether by
// value, by reference or const.
template <typename T>
using RemoveCvRef = RemoveCv<RemoveReference<T>>;

// std::is_same: kValue is whether A and B are one type.
template <typename A, typename B>
struct IsSame {
  static constexpr bool kValue{false};
};
template <typename A>
struct IsSame<A, A> {
  static constexpr bool kValue{true};
};

// std::is_void: kValue is whether T is void, const or not; for a function
// passed only to look, whose result would otherwise be dropped unseen.
template <typename T>
struct IsVoid {
  static constexpr bool kValue{IsSame<RemoveCv<T>, void>::kValue};
};

// std::is_lvalue_reference: kValue is whether T is a T&, for a function that
// has to return a reference to something that outlives the call.
template <typename T>
struct IsLvalueReference {
  static constexpr bool kValue{false};
};
template <typename T>
struct IsLvalueReference<T&> {
  static constexpr bool kValue{true};
};

// std::enable_if_t: void, if B is true; no type, and so no overload, if not.
template <bool B>
struct EnableIfImpl {};
template <>
struct EnableIfImpl<true> {
  using Type = void;
};
template <bool B>
using EnableIf = typename EnableIfImpl<B>::Type;

// std::is_pointer: kValue is whether T is a pointer, const or not, to an
// object or to a function.
template <typename T>
struct IsPointer {
  static constexpr bool kValue{false};
};
template <typename T>
struct IsPointer<T*> {
  static constexpr bool kValue{true};
};
template <typename T>
struct IsPointer<T* const> {
  static constexpr bool kValue{true};
};

// std::is_const: kValue is whether T is const at the top.
template <typename T>
struct IsConst {
  static constexpr bool kValue{false};
};
template <typename T>
struct IsConst<T const> {
  static constexpr bool kValue{true};
};

// std::is_function: kValue is whether T is a function type, such as
// void(int). A const on a function type or a reference is dropped, so those
// two alone are not const with one added.
template <typename T>
struct IsFunction {
  static constexpr bool kValue{!IsConst<T const>::kValue &&
                               IsSame<RemoveReference<T>, T>::kValue};
};

// std::is_member_pointer: kValue is whether T is a pointer to a member, of a
// function or of data.
template <typename T>
struct IsMemberPointer {
  static constexpr bool kValue{false};
};
template <typename M, typename C>
struct IsMemberPointer<M C::*> {
  static constexpr bool kValue{true};
};

// The type of a template parameter that takes a value of type T: a pointer
// to the function for a function type, which a parameter cannot have, and T
// without its const otherwise, as a parameter drops it.
template <typename T, bool = IsFunction<T>::kValue>
struct TemplateArgumentImpl {
  using Type = RemoveCv<T>;
};
template <typename T>
struct TemplateArgumentImpl<T, true> {
  using Type = T*;
};
template <typename T>
using TemplateArgument = typename TemplateArgumentImpl<T>::Type;

/*
 * The test of std::is_invocable_r for what is not a pointer to a member:
 * void if an F& can be called with arguments of Args and its result converts
 * to R implicitly, or R is void, which takes any result; no type if not.
 */
template <typename R>
struct ResultConvertsTo {
  template <typename Result>
  using Type = decltype(ConvertTo<R>(DeclVal<Result>()));
};
template <>
struct ResultConvertsTo<void> {
  template <typename Result>
  using Type = Void<Result>;
};
template <typename R, typename F, typename... Args>
using CallableAs =
    typename ResultConvertsTo<R>::template Type<decltype(DeclVal<F&>()(
        DeclVal<Args>()...))>;

// A pointer to an array of T of unknown size: the type the test below works
// with, not an array that wants to be a utl::array.
template <typename T>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
using ArrayPointer = T (*)[];

/*
 * Names a type only if an array of From may be seen as an array of To, the
 * test std::span makes: it adds const or volatile, as int to const int, but
 * it never turns Derived into Base, whose elements would lie at other
 * distances, nor long into int.
 */
template <typename From, typename To>
using ArrayConvertible =
    decltype(ConvertTo<ArrayPointer<To>>(DeclVal<ArrayPointer<From>>()));

}  // namespace internal
}  // namespace utl

#endif  // UTL_SRC_UTL_INTERNAL_TYPE_TRAITS_H_
