// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief A vector of at most N elements, kept inside: std::inplace_vector, from
 *        C++26, for gnu++11.
 *
 * What would throw when full returns a Status instead.
 */

#ifndef UTL_SRC_UTL_INPLACE_VECTOR_H_
#define UTL_SRC_UTL_INPLACE_VECTOR_H_

#include <stddef.h>
#include <stdint.h>

#include "utl/index_error.h"
#include "utl/internal/in_place.h"
#include "utl/internal/initializer_list.h"
#include "utl/internal/type_traits.h"
#include "utl/misuse.h"
#include "utl/result.h"
#include "utl/status.h"

namespace utl {

/*! \cond */
// What a sketch does not see: how the elements are kept.
namespace internal {

// The smallest unsigned type that counts from 0 to N. 0xFF, not UINT8_MAX,
// which avr-libc defines for C++ only with __STDC_LIMIT_MACROS.
template <size_t N, bool = (N <= 0xFF)>
struct CountForImpl {
  using Type = size_t;
};
template <size_t N>
struct CountForImpl<N, true> {
  using Type = uint8_t;
};
template <size_t N>
using CountFor = typename CountForImpl<N>::Type;

/*
 * Room for N elements, none of them made: a union, so that a constexpr
 * constructor makes it by its byte alone, an inplace_vector made when
 * compiled, and the elements are made one by one into it. Elements that are
 * not trivially copyable need a destructor of the union, which then does
 * nothing: the inplace_vector destroys what it made.
 */
template <typename T, size_t N, bool = __is_trivially_copyable(T)>
union VectorStorage {
  constexpr VectorStorage() : none() {}

  uint8_t none;
  T elements[N];  // NOLINT(modernize-avoid-c-arrays)
};
template <typename T, size_t N>
union VectorStorage<T, N, false> {
  constexpr VectorStorage() : none() {}
  // NOLINTNEXTLINE(modernize-use-equals-default): = default would be deleted.
  ~VectorStorage() {}

  uint8_t none;
  T elements[N];  // NOLINT(modernize-avoid-c-arrays)
};

/*
 * The storage and the number of elements. For elements that are trivially
 * copyable, copying, moving and destroying are those of the compiler, and an
 * inplace_vector of them is trivially copyable itself: its bytes are copied,
 * those of the elements not made among them.
 */
template <typename T, size_t N, bool = __is_trivially_copyable(T)>
struct VectorBase {
  constexpr VectorBase() : storage(), length(0) {}

  VectorStorage<T, N> storage;
  CountFor<N> length;
};

// For elements that are not: each is copied, moved and destroyed itself.
// A struct, all public, as an inplace_vector derives from it privately and
// a sketch sees nothing of it.
template <typename T, size_t N>
struct VectorBase<T, N, false> {
  constexpr VectorBase() : storage(), length(0) {}

  VectorBase(VectorBase const& other) : storage(), length(0) {
    CopyFrom(other);
  }

  VectorBase(VectorBase&& other) : storage(), length(0) { MoveFrom(other); }

  VectorBase& operator=(VectorBase const& other) {
    if (this != &other) {
      DestroyAll();
      CopyFrom(other);
    }
    return *this;
  }

  VectorBase& operator=(VectorBase&& other) {
    if (this != &other) {
      DestroyAll();
      MoveFrom(other);
    }
    return *this;
  }

  ~VectorBase() { DestroyAll(); }

  VectorStorage<T, N> storage;
  CountFor<N> length;

  void CopyFrom(VectorBase const& other) {
    for (; length != other.length; ++length) {
      ::new (InPlace{&storage.elements[length]})
          T(other.storage.elements[length]);
    }
  }

  // The elements of other are moved from, and stay, as std::inplace_vector
  // leaves them.
  void MoveFrom(VectorBase& other) {
    for (; length != other.length; ++length) {
      ::new (InPlace{&storage.elements[length]})
          T(static_cast<T&&>(other.storage.elements[length]));
    }
  }

  void DestroyAll() {
    while (length != 0) {
      --length;
      storage.elements[length].~T();
    }
  }
};

}  // namespace internal
/*! \endcond */

/*!
 * \brief A vector of at most `N` elements of `T`, kept inside, never on the
 *        heap.
 *
 * It grows and shrinks as a std::vector does, but its room is fixed: `N`
 * elements and a count, 1 byte for `N` up to 255. An
 * `inplace_vector<uint8_t, 8>` is 9 bytes on the Uno. What would need more room
 * returns IndexError::kFull:
 *
 * \code{.cpp}
 * utl::inplace_vector<uint8_t, 8> pins{2, 3, 4};
 * pins.push_back(5);                          // a Status: ok, or kFull
 * for (uint8_t const pin : pins) { ... }
 * pins.at(9).ValueOr(0);                      // a Result, as of an array
 * Sum(pins);                                  // a span<uint8_t const>
 * \endcode
 *
 * Elements are made when added and destroyed when removed, so `T` needs no
 * default constructor. One made empty, or of elements that are trivially
 * copyable, is a constant when compiled: a global one costs no code to set
 * up. One made of a braced list is set up when the sketch starts, with the
 * list in RAM as well; a table that never changes is cheaper as a `constexpr`
 * utl::array.
 *
 * \tparam T The type of the elements, not `const`.
 * \tparam N The most elements it holds, at least 1; checked when compiled.
 *
 * \stddiff What adds an element, push_back(), emplace_back(), insert(),
 *          emplace() and resize(), returns a Status with IndexError::kFull
 *          where the standard throws std::bad_alloc, and no reference or
 *          iterator, as a Status, unlike a Result, may be dropped unseen. at()
 *          returns a Result where it throws std::out_of_range. A braced list of
 *          more than `N` keeps the first `N`, reported with `UTL_CHECK_ACCESS`.
 *          No `try_` and `unchecked_` variants, which push_back() covers; no
 *          constructor of `n` elements, which resize() does; no `reserve()`,
 *          `shrink_to_fit()`, `swap()`, `assign()`, `append_range()` or `<`. No
 *          inplace_vector of no elements.
 */
template <typename T, size_t N>
class inplace_vector : private internal::VectorBase<T, N> {
  static_assert(N > 0, "utl::inplace_vector needs room for an element");
  static_assert(!internal::IsConst<T>::kValue,
                "utl::inplace_vector: T is not const, as its elements change");

 public:
  using value_type = T;             /*!< The type of an element. */
  using size_type = size_t;         /*!< The type of a size. */
  using reference = T&;             /*!< A reference to an element. */
  using const_reference = T const&; /*!< One that cannot write. */
  using pointer = T*;               /*!< A pointer to an element. */
  using const_pointer = T const*;   /*!< One that cannot write. */
  using iterator = T*;              /*!< An iterator, a pointer. */
  using const_iterator = T const*;  /*!< One that cannot write. */

  /*! \brief Makes an empty one. */
  constexpr inplace_vector() {}  // NOLINT(modernize-use-equals-default)

  /*!
   * \brief Makes one of the elements of a braced list, `{2, 3, 4}`.
   *
   * Implicit, as in the standard, so that `= {2, 3, 4}` works. A constant that
   * does not fit `T`, `300` for a `uint8_t`, is narrowing, as in the standard.
   *
   * \param[in] values The elements, at most `N`.
   * \pre `values` holds at most `N` elements. Of more, the first `N` are kept;
   *      with `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  inplace_vector(std::initializer_list<T> values) {  // NOLINT(runtime/explicit)
    Append(values);
  }

  /*!
   * \brief Replaces the elements with those of a braced list.
   *
   * \param[in] values The elements, at most `N`.
   * \return This one.
   * \pre `values` holds at most `N` elements, as for the constructor.
   */
  inplace_vector& operator=(std::initializer_list<T> values) {
    clear();
    Append(values);
    return *this;
  }

  /*! \brief Returns the number of elements. */
  constexpr size_type size() const { return this->length; }

  /*! \brief Returns whether it holds no element. */
  constexpr bool empty() const { return this->length == 0; }

  /*! \brief Returns the most elements it holds, `N`. */
  static constexpr size_type capacity() { return N; }

  /*! \brief Returns the most elements it holds, `N`, as capacity() does. */
  static constexpr size_type max_size() { return N; }

  /*!
   * \brief Returns the element at `index`, unchecked.
   *
   * \param[in] index The position, from 0.
   * \return The element.
   * \pre `index` < size(). Beyond the end the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kIndex.
   */
  reference operator[](size_type index) {
    return static_cast<void>(
               internal::Check(index < this->length, Misuse::kIndex)),
           Elements()[index];
  }

  /*! \copydoc operator[](size_type) */
  constexpr const_reference operator[](size_type index) const {
    return static_cast<void>(
               internal::Check(index < this->length, Misuse::kIndex)),
           Elements()[index];
  }

  /*!
   * \brief Returns the element at `index`, or IndexError::kOutOfRange.
   *
   * A copy, as a Result holds a value. To write, check the index and use
   * operator[].
   *
   * \param[in] index The position, from 0.
   * \return The element, or IndexError::kOutOfRange if `index` >= size().
   *
   * \stddiff Returns a Result where std::inplace_vector::at() throws
   *          std::out_of_range.
   */
  Result<T> at(size_type index) const {
    if (index >= this->length) {
      return IndexError::kOutOfRange;
    }
    return Elements()[index];
  }

  /*!
   * \brief Returns the first element, unchecked.
   *
   * \return The first element.
   * \pre It is not empty(). If it is, the access is undefined; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   */
  reference front() {
    return static_cast<void>(
               internal::Check(this->length != 0, Misuse::kEmpty)),
           Elements()[0];
  }

  /*! \copydoc front() */
  constexpr const_reference front() const {
    return static_cast<void>(
               internal::Check(this->length != 0, Misuse::kEmpty)),
           Elements()[0];
  }

  /*!
   * \brief Returns the last element, unchecked.
   *
   * \return The last element.
   * \pre It is not empty(), as for front().
   */
  reference back() {
    return static_cast<void>(
               internal::Check(this->length != 0, Misuse::kEmpty)),
           Elements()[this->length - 1];
  }

  /*! \copydoc back() */
  constexpr const_reference back() const {
    return static_cast<void>(
               internal::Check(this->length != 0, Misuse::kEmpty)),
           Elements()[this->length - 1];
  }

  /*! \brief Returns the address of the first element. */
  pointer data() { return Elements(); }

  /*! \copydoc data() */
  constexpr const_pointer data() const { return Elements(); }

  /*! \brief Returns an iterator to the first element. */
  iterator begin() { return Elements(); }

  /*! \copydoc begin() */
  constexpr const_iterator begin() const { return Elements(); }

  /*! \brief Returns an iterator to the first element that cannot write. */
  constexpr const_iterator cbegin() const { return Elements(); }

  /*! \brief Returns an iterator past the last element. */
  iterator end() { return Elements() + this->length; }

  /*! \copydoc end() */
  constexpr const_iterator end() const { return Elements() + this->length; }

  /*! \brief Returns an iterator past the last element that cannot write. */
  constexpr const_iterator cend() const { return Elements() + this->length; }

  /*!
   * \brief Adds a copy of `value` at the end, or returns IndexError::kFull.
   *
   * \param[in] value The element to add.
   * \return Ok, or IndexError::kFull if it holds `N` elements already.
   *
   * \stddiff Returns a Status where std::inplace_vector::push_back() returns a
   *          reference to the element and throws std::bad_alloc when full.
   */
  Status push_back(T const& value) { return emplace_back(value); }

  /*!
   * \brief Moves `value` to the end, or returns IndexError::kFull.
   *
   * \param[in] value The element to add.
   * \return Ok, or IndexError::kFull if it holds `N` elements already.
   */
  Status push_back(T&& value) { return emplace_back(static_cast<T&&>(value)); }

  /*!
   * \brief Makes an element of `args` at the end, or returns IndexError::kFull.
   *
   * \param[in] args What the constructor of `T` takes.
   * \return Ok, or IndexError::kFull if it holds `N` elements already.
   */
  template <typename... Args>
  Status emplace_back(Args&&... args) {
    if (this->length == N) {
      return IndexError::kFull;
    }
    ::new (internal::InPlace{end()}) T(static_cast<Args&&>(args)...);
    ++this->length;
    return Status::Ok();
  }

  /*!
   * \brief Adds a copy of `value` before `position`, or returns an error.
   *
   * The elements from `position` on move one place on. The new one lies at
   * `position`.
   *
   * \param[in] position Where it goes, from begin() to end().
   * \param[in] value The element to add.
   * \return Ok, IndexError::kOutOfRange if `position` lies outside, or
   *         IndexError::kFull if it holds `N` elements already.
   *
   * \stddiff Returns a Status where std::inplace_vector::insert() returns an
   *          iterator and throws std::bad_alloc when full.
   */
  Status insert(const_iterator position, T const& value) {
    return emplace(position, value);
  }

  /*!
   * \brief Moves `value` before `position`, or returns an error.
   *
   * \param[in] position Where it goes, from begin() to end().
   * \param[in] value The element to add.
   * \return Ok, IndexError::kOutOfRange if `position` lies outside, or
   *         IndexError::kFull if it holds `N` elements already.
   */
  Status insert(const_iterator position, T&& value) {
    return emplace(position, static_cast<T&&>(value));
  }

  /*!
   * \brief Makes an element of `args` before `position`, or returns an error.
   *
   * \param[in] position Where it goes, from begin() to end().
   * \param[in] args What the constructor of `T` takes.
   * \return Ok, IndexError::kOutOfRange if `position` lies outside, or
   *         IndexError::kFull if it holds `N` elements already.
   */
  template <typename... Args>
  Status emplace(const_iterator position, Args&&... args) {
    // As unsigned, a position before begin() lies far beyond the end too.
    auto const index = static_cast<size_type>(position - cbegin());
    if (index > this->length) {
      return IndexError::kOutOfRange;
    }
    if (this->length == N) {
      return IndexError::kFull;
    }
    // Made first, as an argument may refer to an element that moves.
    T value(static_cast<Args&&>(args)...);
    T* const target = begin() + index;
    if (target == end()) {
      ::new (internal::InPlace{end()}) T(static_cast<T&&>(value));
    } else {
      // The last moves into room not made yet, the others into elements.
      ::new (internal::InPlace{end()}) T(static_cast<T&&>(end()[-1]));
      for (pointer to = end() - 1; to != target; --to) {
        *to = static_cast<T&&>(to[-1]);
      }
      *target = static_cast<T&&>(value);
    }
    ++this->length;
    return Status::Ok();
  }

  /*!
   * \brief Removes the last element.
   *
   * \pre It is not empty(). If it is, nothing is removed; with
   *      `UTL_CHECK_ACCESS` at 1 it is reported first, as Misuse::kEmpty.
   *
   * \stddiff Does nothing when empty, where the standard leaves it undefined.
   */
  void pop_back() {
    if (internal::Check(this->length != 0, Misuse::kEmpty)) {
      DestroyFrom(this->length - 1);
    }
  }

  /*!
   * \brief Removes the element at `position`.
   *
   * The elements after it move one place back.
   *
   * \param[in] position The element, from begin() to before end().
   * \return An iterator to the element that follows the one removed.
   * \pre `position` points to an element. If not, nothing is removed and end()
   *      is returned; with `UTL_CHECK_ACCESS` at 1 it is reported first, as
   *      Misuse::kIndex.
   */
  iterator erase(const_iterator position) {
    return erase(position, position + 1);
  }

  /*!
   * \brief Removes the elements from `first` to before `last`.
   *
   * \param[in] first The first element to remove.
   * \param[in] last The element after the last to remove, or end().
   * \return An iterator to the element that follows those removed.
   * \pre begin() <= `first` <= `last` <= end(). If not, nothing is removed and
   *      end() is returned; with `UTL_CHECK_ACCESS` at 1 it is reported first,
   *      as Misuse::kIndex.
   */
  iterator erase(const_iterator first, const_iterator last) {
    // As unsigned, a position before begin() lies far beyond the end too.
    auto const from = static_cast<size_type>(first - cbegin());
    auto const to = static_cast<size_type>(last - cbegin());
    if (!internal::Check(from <= to && to <= this->length, Misuse::kIndex)) {
      return end();
    }
    pointer kept = begin() + from;
    for (pointer moved = begin() + to; moved != end(); ++moved, ++kept) {
      *kept = static_cast<T&&>(*moved);
    }
    DestroyFrom(static_cast<size_type>(kept - begin()));
    return begin() + from;
  }

  /*! \brief Removes every element. */
  void clear() { DestroyFrom(0); }

  /*!
   * \brief Makes it hold `count` elements: removes those beyond, or adds
   *        elements made by `T()`.
   *
   * \param[in] count The number of elements, at most `N`.
   * \return Ok, or IndexError::kFull if `count` > `N`, which leaves it as it
   *         was.
   *
   * \stddiff Returns a Status where std::inplace_vector::resize() throws
   *          std::bad_alloc.
   */
  Status resize(size_type count) { return ResizeWith(count); }

  /*!
   * \brief Makes it hold `count` elements: removes those beyond, or adds copies
   *        of `value`.
   *
   * \param[in] count The number of elements, at most `N`.
   * \param[in] value What an element added is a copy of.
   * \return Ok, or IndexError::kFull if `count` > `N`, which leaves it as it
   *         was.
   */
  Status resize(size_type count, T const& value) {
    return ResizeWith(count, value);
  }

 private:
  pointer Elements() { return this->storage.elements; }
  constexpr const_pointer Elements() const { return this->storage.elements; }

  // Destroys the elements from `count` on, from the last.
  void DestroyFrom(size_type count) {
    while (this->length > count) {
      --this->length;
      Elements()[this->length].~T();
    }
  }

  void Append(std::initializer_list<T> values) {
    static_cast<void>(internal::Check(values.size() <= N, Misuse::kIndex));
    size_type const count = values.size() < N ? values.size() : N;
    for (T const* value = values.begin(); this->length != count; ++value) {
      ::new (internal::InPlace{end()}) T(*value);
      ++this->length;
    }
  }

  template <typename... Args>
  Status ResizeWith(size_type count, Args const&... args) {
    if (count > N) {
      return IndexError::kFull;
    }
    DestroyFrom(count);
    while (this->length != count) {
      ::new (internal::InPlace{end()}) T(args...);
      ++this->length;
    }
    return Status::Ok();
  }
};

/*!
 * \brief Returns whether `a` and `b` hold as many elements, and equal ones, one
 *        by one.
 *
 * \param[in] a An inplace_vector.
 * \param[in] b Another of the same type and capacity.
 */
template <typename T, size_t N>
bool operator==(inplace_vector<T, N> const& a, inplace_vector<T, N> const& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t index = 0; index < a.size(); ++index) {
    if (!(a[index] == b[index])) {
      return false;
    }
  }
  return true;
}

/*!
 * \brief Returns whether `a` and `b` differ in size or in an element.
 *
 * \param[in] a An inplace_vector.
 * \param[in] b Another of the same type and capacity.
 */
template <typename T, size_t N>
bool operator!=(inplace_vector<T, N> const& a, inplace_vector<T, N> const& b) {
  return !(a == b);
}

}  // namespace utl

#endif  // UTL_SRC_UTL_INPLACE_VECTOR_H_
