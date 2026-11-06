// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A reference to something to call: std::function_ref, from C++26, for
 *        gnu++11.
 */

#ifndef UTL_SRC_UTL_FUNCTION_REF_H_
#define UTL_SRC_UTL_FUNCTION_REF_H_

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
 * uint8_t on = 0;
 * ForEachLed([&on](Led& led) { on += led.IsOn(); });
 * \endcode
 *
 * It refers to a lambda or an object, never copies it, so what it refers to
 * has to outlive every call. A lambda written as the argument lives until
 * the call returns: as a parameter it is safe, kept in a member or a variable
 * it is gone before the next call. A function lives for good: one of a
 * function, as `utl::function_ref<void()> f = Blink`, may be kept.
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
 *          no type before C++17. No constructor that takes the function as a
 *          template argument, for a member function bound to its object, as it
 *          needs C++17's `template <auto>`: a lambda that calls the member,
 *          kept beside the function_ref, does the same. A function pointer that
 *          is null is reported with `UTL_CHECK_ACCESS`, where the standard
 *          leaves it undefined.
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
      : bound_(reinterpret_cast<void (*)()>(f)), call_(&CallFunction<F>) {
    // reinterpret_cast, not constexpr: the one way to keep a pointer to a
    // function of any type, and CallFunction<F> casts it back to an F*.
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
      : bound_(const_cast<void*>(
            static_cast<void const*>(__builtin_addressof(f)))),
        call_(&CallObject<T>) {
    // The EnableIf leaves a function_ref to the copy constructor; clang-tidy
    // sees only std::enable_if, which avr-gcc lacks.
    // __builtin_addressof, as an object may overload unary &. The const goes
    // in the pointer, and CallObject<T> puts it back, T being const then.
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
    constexpr explicit Bound(void* object_in) : object(object_in) {}
    constexpr explicit Bound(void (*function_in)()) : function(function_in) {}

    void* object;
    void (*function)();
  };

  template <typename F>
  static R CallFunction(Bound bound, Args... args) {
    return internal::CallReturning<R>::Call(
        *reinterpret_cast<F*>(bound.function), static_cast<Args&&>(args)...);
  }

  template <typename T>
  static R CallObject(Bound bound, Args... args) {
    return internal::CallReturning<R>::Call(*static_cast<T*>(bound.object),
                                            static_cast<Args&&>(args)...);
  }

  Bound bound_;
  R (*call_)(Bound, Args...);
};

}  // namespace utl

#endif  // UTL_SRC_UTL_FUNCTION_REF_H_
