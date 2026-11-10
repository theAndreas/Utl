// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A reference to something to call: std::function_ref, from C++26, for
 *        gnu++11.
 */

#ifndef UTL_SRC_UTL_FUNCTION_REF_H_
#define UTL_SRC_UTL_FUNCTION_REF_H_

#include "utl/constant_wrapper.h"
#include "utl/internal/invoke.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"

namespace utl {

/*!
 * \brief Stops the build: `Signature` has to be a function type, such as
 *        `void(uint8_t)`.
 *
 * \tparam Signature How it is called: what it returns, and the types of its
 *                   arguments.
 */
template <typename Signature>
class function_ref {
  static_assert(!internal::IsSame<Signature, Signature>::kValue,
                "function_ref: Signature is a function type, such as "
                "void(uint8_t)");
};

/*!
 * \brief A reference to something to call with `Args` that returns an `R`.
 *
 * It refers to a function, a lambda or an object with `operator()`, and owns
 * nothing: 4 bytes on the Uno, a pointer to what it refers to and one to the
 * code that calls it, without the heap or a virtual function. It is meant for
 * a parameter, as a function that takes it is no template, and takes any of
 * them, with its captures:
 *
 * \code{.cpp}
 * void ForEachLed(utl::function_ref<void(Led&)> f) {
 *   for (Led& led : leds) {
 *     f(led);
 *   }
 * }
 *
 * uint8_t on{0};
 * ForEachLed([&on](Led& led) { on += led.IsOn(); });
 * \endcode
 *
 * It refers to a lambda or an object, never copies it, so what it refers to
 * has to outlive every call. A lambda written as the argument lives until
 * the call returns: as a parameter it is safe, kept in a member or a variable
 * it is gone before the next call. A function lives for good: one of a
 * function, as `utl::function_ref<void()> f = Blink`, may be kept.
 *
 * A member function is bound to its object with UTL_CW(), the function given
 * as a constant: the function_ref refers to the object, not to a lambda, and
 * may be kept as long as the object lives, as a callback in a member:
 *
 * \code{.cpp}
 * utl::function_ref<void()> on_press{UTL_CW(&Led::Toggle), led};
 * ForEachLed(UTL_CW(&Led::Toggle));  // a function_ref<void(Led&)>
 * \endcode
 *
 * There is no default constructor, so one is never empty, and so no
 * optional of one, which would need it: where nothing is to be called, it
 * refers to a function that does nothing. Assigning another rebinds it.
 *
 * \tparam R What a call returns. `void` takes any result and drops it.
 * \tparam Args The types of the arguments.
 *
 * \stddiff Neither a signature with `const` nor one with `noexcept`, which
 *          gnu++11 cannot have: the one only narrows what binds, the other is
 *          no type before C++17. A function given as a constant is written
 *          `UTL_CW(f)`, not `std::cw<f>`, as gnu++11 has no `template <auto>`,
 *          and is a function or a member function, never a pointer to a data
 *          member. A function pointer that is null, and so a null pointer to
 *          the object of a member function, is reported with
 *          `UTL_CHECK_ACCESS`, where the standard leaves it undefined.
 */
template <typename R, typename... Args>
class function_ref<R(Args...)> {
 public:
  /*!
   * \brief Refers to the function `f` points to, or to `f` itself if given by
   *        name.
   *
   * Implicit, as in the standard, so that a function passes where a
   * function_ref is taken. A function whose types differ binds as well, if
   * the arguments convert to its parameters and its result to an `R`.
   *
   * \param[in] f The function.
   *
   * \pre `f` is not null. A call through a null one jumps to address 0, which
   *      restarts the sketch on the AVR; with `UTL_CHECK_ACCESS` at 1 it is
   *      reported as Misuse::kNullFunction when the function_ref is made.
   */
  template <typename F,
            typename = internal::EnableIf<internal::IsFunction<F>::kValue>,
            typename = internal::CallableAs<R, F, Args...>>
  function_ref(F* f)  // NOLINT(runtime/explicit)
      : bound_{ToFunction(f)}, call_{&CallFunction<F>} {
    // Not constexpr, as ToFunction() is not.
    static_cast<void>(internal::Check(f != nullptr, Misuse::kNullFunction));
  }

  /*!
   * \brief Refers to `f`, a lambda or an object with `operator()`.
   *
   * Implicit, as in the standard, so that a lambda passes where a
   * function_ref is taken. It binds a temporary as well, for a parameter;
   * it is gone after the call. Calls go to `f` as an lvalue, and to its
   * `const` `operator()` if `f` is `const`.
   *
   * \param[in] f What to call: an lvalue that outlives the function_ref, or a
   *              temporary written as the argument.
   */
  template <typename F, typename T = internal::RemoveReference<F>,
            typename = internal::EnableIf<
                !internal::IsFunction<T>::kValue &&
                !internal::IsSame<internal::RemoveCv<T>, function_ref>::kValue>,
            typename = internal::CallableAs<R, T, Args...>>
  // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
  constexpr function_ref(F&& f)  // NOLINT(runtime/explicit)
      : bound_{Unconst(__builtin_addressof(f))}, call_{&CallObject<T>} {
    // The EnableIf leaves a function_ref to the copy constructor; clang-tidy
    // sees only std::enable_if, which avr-gcc lacks.
    // __builtin_addressof, as an object may overload unary &.
  }

  /*!
   * \brief Refers to `Function`, given as a constant: a function, or a member
   *        function called on the first argument.
   *
   * Implicit, as in the standard. The function is part of the code that
   * calls it, so a member function is called with its object as the first of
   * `Args`, an object or a pointer to one:
   *
   * \code{.cpp}
   * void ForEachLed(utl::function_ref<void(Led&)> f);
   * ForEachLed(UTL_CW(&Led::Toggle));
   * \endcode
   */
  template <typename F, F Function,
            typename = internal::InvocableAs<R, F, Args...>>
  constexpr function_ref(
      constant_wrapper<F, Function> /*f*/)  // NOLINT(runtime/explicit)
      : bound_{static_cast<void*>(nullptr)}, call_{&CallConstant<F, Function>} {
    static_assert(Function != nullptr, "function_ref: the function is null");
  }

  /*!
   * \brief Refers to `Function`, given as a constant, bound to `object`: a
   *        member function of it, or a function that takes it first.
   *
   * \code{.cpp}
   * utl::function_ref<void()> const toggle{UTL_CW(&Led::Toggle), led};
   * \endcode
   *
   * Calls go to `Function` with `object` in front of `args`, and to a
   * `const` member function if `object` is `const`.
   *
   * \param[in] object What it is bound to: an lvalue that outlives the
   *                   function_ref, never a temporary.
   */
  template <typename F, F Function, typename T,
            typename = internal::InvocableAs<R, F, T&, Args...>>
  constexpr function_ref(constant_wrapper<F, Function> /*f*/, T& object)
      : bound_{Unconst(__builtin_addressof(object))},
        call_{&CallConstantOn<F, Function, T>} {
    static_assert(Function != nullptr, "function_ref: the function is null");
  }

  /*!
   * \brief Refers to `Function`, given as a constant, bound to the pointer
   *        `object`: a member function of what it points to, or a function that
   *        takes the pointer first.
   *
   * \code{.cpp}
   * utl::function_ref<void()> const toggle{UTL_CW(&Led::Toggle), &led};
   * \endcode
   *
   * It keeps the pointer itself, not where it is stored.
   *
   * \param[in] object The pointer it is bound to.
   *
   * \pre `object` is not null if `Function` is a member function. With
   *      `UTL_CHECK_ACCESS` at 1 a null one is reported as
   *      Misuse::kNullFunction when the function_ref is made.
   */
  template <typename F, F Function, typename T,
            typename = internal::InvocableAs<R, F, T*, Args...>>
  function_ref(constant_wrapper<F, Function> /*f*/, T* object)
      : bound_{Unconst(object)}, call_{&CallConstantWith<F, Function, T>} {
    static_assert(Function != nullptr, "function_ref: the function is null");
    static_cast<void>(internal::Check(
        !internal::IsMemberPointer<F>::kValue || object != nullptr,
        Misuse::kNullFunction));
  }

  /*!
   * \brief Deleted: a lambda or an object assigned would be gone before the
   *        next call.
   *
   * Assigning another function_ref, or a function, rebinds it:
   * `f = utl::function_ref<void()>(blink)` or `f = Blink`.
   */
  template <typename T, typename = internal::EnableIf<
                            !internal::IsSame<T, function_ref>::kValue &&
                            !internal::IsPointer<T>::kValue>>
  function_ref& operator=(T) = delete;

  /*!
   * \brief Calls what it refers to with `args`.
   *
   * \param[in] args The arguments, passed on as taken.
   * \return What the call returns, converted to an `R`.
   */
  R operator()(Args... args) const {
    return call_(bound_, static_cast<Args&&>(args)...);
  }

 private:
  // What it is bound to: an object or a function, whose pointers may differ
  // in size and are never converted into each other.
  union Bound {
    constexpr explicit Bound(void* object_in) : object{object_in} {}
    constexpr explicit Bound(void (*function_in)()) : function{function_in} {}

    void* object;
    void (*function)();
  };

  // The address of an object of any type, const or not, as one type. The
  // const goes in the pointer, and the Call function puts it back, T being
  // const then.
  static constexpr void* Unconst(void const* object) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<void*>(object);
  }

  // A pointer to a function of any type, as one type: the one way to keep it,
  // and so not constexpr. CallFunction<F> casts it back to an F*.
  using AnyFunction = void (*)();
  template <typename F>
  static AnyFunction ToFunction(F* f) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<AnyFunction>(f);
  }

  template <typename F>
  static R CallFunction(Bound bound, Args... args) {
    return internal::CallReturning<R>::Call(
        // Back to the F* the constructor kept.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<F*>(bound.function), static_cast<Args&&>(args)...);
  }

  template <typename T>
  static R CallObject(Bound bound, Args... args) {
    return internal::CallReturning<R>::Call(*static_cast<T*>(bound.object),
                                            static_cast<Args&&>(args)...);
  }

  // The function, a template argument, is a constant here: the call is
  // direct, and no pointer to it is kept.
  template <typename F, F Function>
  static R CallConstant(Bound /*bound*/, Args... args) {
    return internal::CallReturning<R>::Invoke(Function,
                                              static_cast<Args&&>(args)...);
  }

  template <typename F, F Function, typename T>
  static R CallConstantOn(Bound bound, Args... args) {
    return internal::CallReturning<R>::Invoke(
        Function, *static_cast<T*>(bound.object), static_cast<Args&&>(args)...);
  }

  template <typename F, F Function, typename T>
  static R CallConstantWith(Bound bound, Args... args) {
    return internal::CallReturning<R>::Invoke(
        Function, static_cast<T*>(bound.object), static_cast<Args&&>(args)...);
  }

  Bound bound_;
  R (*call_)(Bound, Args...);
};

}  // namespace utl

#endif  // UTL_SRC_UTL_FUNCTION_REF_H_
