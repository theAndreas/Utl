// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Something to call, kept inside: std::function without the heap, for
 *        gnu++11.
 */

#ifndef UTL_SRC_UTL_INPLACE_FUNCTION_H_
#define UTL_SRC_UTL_INPLACE_FUNCTION_H_

#include <stddef.h>
#include <stdint.h>

#include "utl/array.h"
#include "utl/internal/in_place.h"
#include "utl/internal/invoke.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"

namespace utl {

/*! \cond */
// What a sketch does not see: the alignment of the storage, and what is kept
// of a callable.
namespace internal {

// std::max_align_t, without long double, which Google's style keeps out: as
// strictly aligned as any scalar, which is 1 on the AVR.
union MaxAlign {
  int64_t integer;
  double floating;
  void* object;
  void (*function)();
};

// The type a callable is kept as: a function as a pointer to it, anything
// else without its const or volatile, as std::decay_t.
template <typename F>
struct StoredImpl {
  using Type = RemoveCvRef<F>;
};
template <typename R, typename... Args>
struct StoredImpl<R(Args...)> {
  using Type = R (*)(Args...);
};
template <typename R, typename... Args>
struct StoredImpl<R (&)(Args...)> {
  using Type = R (*)(Args...);
};
template <typename F>
using Stored = typename StoredImpl<F>::Type;

// kValue is whether R() is an expression: R is void, or has a default
// constructor, so that an empty inplace_function has something to return.
// Parentheses, not braces: void() is an expression, void{} only from C++23.
template <typename R, typename = void>
struct IsValueInitializable {
  static constexpr bool kValue{false};
};
template <typename R>
struct IsValueInitializable<R, Void<decltype(R())>> {
  static constexpr bool kValue{true};
};

}  // namespace internal
/*! \endcond */

/*!
 * \brief Stops the build: `Signature` has to be a function type, such as
 *        `void(uint8_t)`.
 *
 * \tparam Signature How it is called: what it returns, and the types of its
 *                   arguments.
 * \tparam Capacity The bytes a callable may take.
 */
template <typename Signature, size_t Capacity = 2 * sizeof(void*)>
class inplace_function {
  static_assert(!internal::IsSame<Signature, Signature>::kValue,
                "inplace_function: Signature is a function type, such as "
                "void(uint8_t)");
};

/*!
 * \brief Something to call with `Args` that returns an `R`, kept inside.
 *
 * It holds a copy of a function, a lambda with its captures or an object with
 * `operator()`, in `Capacity` bytes of its own, never on the heap. A callable
 * larger than that stops the build. On the Uno it is `Capacity` and 4 bytes,
 * 8 by default: room for `[this]` and a byte, or for two references. It is
 * meant for a callback that is kept, which a function_ref would leave
 * dangling:
 *
 * \code{.cpp}
 * class Button {
 *  public:
 *   void OnPress(utl::inplace_function<void()> const& handler) {
 *     on_press_ = handler;
 *   }
 *   void Poll() {
 *     if (Pressed() && on_press_) {
 *       on_press_();
 *     }
 *   }
 *
 *  private:
 *   utl::inplace_function<void()> on_press_;  // empty: nothing to call
 * };
 *
 * button.OnPress([this] { Toggle(); });  // kept, with its capture
 * \endcode
 *
 * It is empty when made without a callable, from `nullptr` or from a null
 * function pointer, and after it was moved from. Copying it copies the
 * callable, so the callable has to be copyable; one that is trivially
 * copyable, as a lambda that captures pointers, references or numbers, is
 * copied as its bytes, without code of its own for it.
 *
 * \tparam R What a call returns: `void`, which takes any result and drops it,
 *           or a type with a default constructor, which a call of an empty one
 *           returns.
 * \tparam Args The types of the arguments.
 * \tparam Capacity The bytes a callable may take, at least 1.
 *
 * \stddiff There is no inplace_function in the standard; this is std::function,
 *          as SG14's inplace_function is, with the callable kept in `Capacity`
 *          bytes rather than on the heap. A call of an empty one returns `R()`
 *          and is reported with `UTL_CHECK_ACCESS`, where std::function throws
 *          std::bad_function_call; R therefore cannot be a reference. Left out
 *          are swap(), the comparisons with `nullptr`, which `operator bool`
 *          does, and target(), which needs RTTI.
 */
template <typename R, typename... Args, size_t Capacity>
class inplace_function<R(Args...), Capacity> {
  static_assert(internal::IsValueInitializable<R>::kValue,
                "inplace_function: R is void or has a default constructor, "
                "for the call of an empty one");
  static_assert(Capacity > 0, "inplace_function: Capacity is at least 1");

 public:
  /*! \brief Makes an empty one. */
  inplace_function() : storage_{}, call_{&CallEmpty}, manage_{nullptr} {}

  /*! \brief Makes an empty one, as `= nullptr` does. */
  inplace_function(decltype(nullptr))  // NOLINT(runtime/explicit)
      : inplace_function{} {}

  /*!
   * \brief Keeps a copy of `f`: a function, a lambda or an object with
   *        `operator()`.
   *
   * Implicit, as in the standard, so that a lambda passes where one is taken.
   * A function whose types differ is kept as well, if the arguments convert
   * to its parameters and its result to an `R`. A null function pointer
   * leaves it empty.
   *
   * \param[in] f What to call, copied or moved inside. It stops the build if it
   *              is larger than `Capacity`.
   */
  template <typename F, typename T = internal::Stored<F>,
            typename = internal::EnableIf<
                !internal::IsSame<T, inplace_function>::kValue>,
            typename = internal::CallableAs<R, T, Args...>>
  // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
  inplace_function(F&& f)  // NOLINT(runtime/explicit)
      : inplace_function{} {
    // The EnableIf leaves an inplace_function to the copy and move
    // constructors; clang-tidy sees only std::enable_if, which avr-gcc lacks.
    Store<T>(static_cast<F&&>(f));
  }

  /*!
   * \brief Copies `other` and the callable it keeps.
   *
   * \param[in] other The one to copy.
   */
  inplace_function(inplace_function const& other) : inplace_function{} {
    CopyFrom(other);
  }

  /*!
   * \brief Moves the callable out of `other`, which is left empty.
   *
   * \param[in,out] other The one to move from.
   */
  inplace_function(inplace_function&& other) : inplace_function{} {
    MoveFrom(other);
  }

  /*! \brief Destroys the callable it keeps. */
  ~inplace_function() { Reset(); }

  /*!
   * \brief Copies `other` and the callable it keeps, in place of its own.
   *
   * \param[in] other The one to copy.
   * \return This one.
   */
  inplace_function& operator=(inplace_function const& other) {
    if (this != &other) {
      Reset();
      CopyFrom(other);
    }
    return *this;
  }

  /*!
   * \brief Moves the callable out of `other`, in place of its own; `other` is
   *        left empty.
   *
   * \param[in,out] other The one to move from.
   * \return This one.
   */
  inplace_function& operator=(inplace_function&& other) {
    if (this != &other) {
      Reset();
      MoveFrom(other);
    }
    return *this;
  }

  /*!
   * \brief Keeps a copy of `f` in place of its own callable.
   *
   * \param[in] f What to call, as for the constructor.
   * \return This one.
   */
  template <typename F, typename T = internal::Stored<F>,
            typename = internal::EnableIf<
                !internal::IsSame<T, inplace_function>::kValue>,
            typename = internal::CallableAs<R, T, Args...>>
  inplace_function& operator=(F&& f) {
    Reset();
    Store<T>(static_cast<F&&>(f));
    return *this;
  }

  /*!
   * \brief Destroys the callable it keeps, and leaves it empty.
   *
   * \return This one.
   */
  inplace_function& operator=(decltype(nullptr)) {
    Reset();
    return *this;
  }

  /*! \brief Returns whether it keeps something to call. */
  explicit operator bool() const { return call_ != &CallEmpty; }

  /*!
   * \brief Calls what it keeps with `args`.
   *
   * `const`, as in std::function, though the callable may change itself, as
   * a lambda that is `mutable` does.
   *
   * \param[in] args The arguments, passed on as taken.
   * \return What the call returns, converted to an `R`; `R()` if empty.
   *
   * \pre It is not empty; ask with `operator bool`. A call of an empty one does
   *      nothing and returns `R()`; with `UTL_CHECK_ACCESS` at 1 it is reported
   *      first, as Misuse::kEmptyFunction.
   */
  R operator()(Args... args) const {
    return call_(storage_.data(), static_cast<Args&&>(args)...);
  }

 private:
  enum class Operation : uint8_t { kCopy, kMove, kDestroy };

  // Anything but a function pointer, kept as it is. Apart from the one below,
  // as a test for null would be a branch in each type that is never taken.
  template <typename T, typename F,
            typename = internal::EnableIf<!internal::IsPointer<T>::kValue>>
  void Store(F&& f) {
    Construct<T>(static_cast<F&&>(f));
  }

  // A function pointer, kept unless null, which leaves it empty, as it does a
  // std::function. The second default only tells it from the one above.
  template <typename T, typename F,
            typename = internal::EnableIf<internal::IsPointer<T>::kValue>,
            typename = void>
  void Store(F&& f) {
    T const pointer{f};
    if (pointer != nullptr) {
      Construct<T>(pointer);
    }
  }

  template <typename T, typename F>
  void Construct(F&& f) {
    static_assert(sizeof(T) <= Capacity,
                  "inplace_function: the callable is larger than Capacity");
    static_assert(alignof(T) <= alignof(internal::MaxAlign),
                  "inplace_function: the callable is aligned more strictly "
                  "than any scalar");
    ::new (internal::InPlace{storage_.data()}) T(static_cast<F&&>(f));
    call_ = &Call<T>;
    // A trivially copyable callable is copied as its bytes, and has no
    // destructor: no Manage<T> for it, which on the AVR is code for each type.
    manage_ = __is_trivially_copyable(T) ? nullptr : &Manage<T>;
  }

  void CopyFrom(inplace_function const& other) {
    if (other.manage_ != nullptr) {
      other.manage_(Operation::kCopy, storage_.data(), other.storage_.data());
    } else {
      storage_ = other.storage_;
    }
    call_ = other.call_;
    manage_ = other.manage_;
  }

  void MoveFrom(inplace_function& other) {
    if (other.manage_ != nullptr) {
      other.manage_(Operation::kMove, storage_.data(), other.storage_.data());
    } else {
      storage_ = other.storage_;
    }
    call_ = other.call_;
    manage_ = other.manage_;
    other.Reset();  // destroys what is left of the moved-from callable
  }

  void Reset() {
    if (manage_ != nullptr) {
      manage_(Operation::kDestroy, storage_.data(), nullptr);
    }
    call_ = &CallEmpty;
    manage_ = nullptr;
  }

  template <typename T>
  static R Call(void* storage, Args... args) {
    return internal::CallReturning<R>::Call(*static_cast<T*>(storage),
                                            static_cast<Args&&>(args)...);
  }

  static R CallEmpty(void* /*storage*/, Args... /*args*/) {
    static_cast<void>(internal::Check(false, Misuse::kEmptyFunction));
    return R();  // not R{}: R may be void, and void{} is C++23
  }

  template <typename T>
  static void Manage(Operation operation, void* to, void* from) {
    // Not a switch, whose default no operation takes.
    if (operation == Operation::kCopy) {
      ::new (internal::InPlace{to}) T(*static_cast<T const*>(from));
    } else if (operation == Operation::kMove) {
      ::new (internal::InPlace{to}) T(static_cast<T&&>(*static_cast<T*>(from)));
    } else {
      static_cast<T*>(to)->~T();
    }
  }

  // Mutable, as operator() is const, as in std::function, and the callable
  // may change itself. Value-initialized, so that an empty one copies bytes
  // that are set.
  alignas(internal::MaxAlign) mutable array<uint8_t, Capacity> storage_;
  R (*call_)(void*, Args...);
  void (*manage_)(Operation, void*, void*);
};

}  // namespace utl

#endif  // UTL_SRC_UTL_INPLACE_FUNCTION_H_
