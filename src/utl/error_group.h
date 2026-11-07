// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The group an error comes from.
 */

#ifndef UTL_SRC_UTL_ERROR_GROUP_H_
#define UTL_SRC_UTL_ERROR_GROUP_H_

#include <Arduino.h>

namespace utl {

/*!
 * \brief The group an error belongs to: the address of its name in flash.
 *
 * A value of 2 bytes that is copied and compared, and that takes no memory of
 * its own. Two groups are equal only if they are the same name, the same
 * array; two groups may share the words of their names.
 *
 * A group is its name, an enum class of its codes, and beside the enum, in
 * the same namespace, a function `GetErrorGroup()` that argument-dependent
 * lookup finds:
 *
 * \code{.cpp}
 * // font_error.h
 * enum class FontError : uint8_t { kUnknownChar, kNullText };
 * extern char const kFontErrorGroup[] PROGMEM;  // "font"
 * constexpr utl::ErrorGroup GetErrorGroup(FontError) {
 *   return utl::ErrorGroup{kFontErrorGroup};
 * }
 *
 * // font_error.cpp
 * char const kFontErrorGroup[] PROGMEM = "font";
 * \endcode
 *
 * \warning Define the name exactly once, in one `.cpp` file, and declare it
 *          `extern` in the header. A `const` array defined in a header gets a
 *          copy in every file that includes it, each at an address of its own,
 *          and errors made in different files would not compare equal.
 *
 * The texts of the codes are not part of the group: a sketch that wants them
 * in words turns them on with UseMessages(), see messages.h.
 */
class ErrorGroup {
 public:
  /*!
   * \brief Makes the group whose name is `name`.
   *
   * \param[in] name The name, an array of chars in `PROGMEM`, defined once.
   */
  constexpr explicit ErrorGroup(char const* name) : name_{name} {}

  /*! \brief Returns the name of the group, from flash. */
  __FlashStringHelper const* Name() const {
    return reinterpret_cast<__FlashStringHelper const*>(name_);
  }

  /*!
   * \brief Returns whether `a` and `b` are the same group.
   *
   * \param[in] a A group.
   * \param[in] b Another group.
   */
  friend constexpr bool operator==(ErrorGroup a, ErrorGroup b) {
    return a.name_ == b.name_;
  }

  /*!
   * \brief Returns whether `a` and `b` are different groups.
   *
   * \param[in] a A group.
   * \param[in] b Another group.
   */
  friend constexpr bool operator!=(ErrorGroup a, ErrorGroup b) {
    return a.name_ != b.name_;
  }

 private:
  friend class Error;

  // No group, which only an Error inside a Status holds, for "ok".
  constexpr explicit ErrorGroup(decltype(nullptr)) : name_{nullptr} {}

  /*
   * The address of the name is what makes a group unique: no one has to hand
   * out group ids, and a component adds a group of its own without changing
   * this library. See docu/ErrorHandling.md of MaxMatrix, "Domains: what
   * makes one unique", for the alternatives that were weighed.
   *
   * A virtual Message(), as before, linked every text of every group into any
   * sketch that made an error, printed or not, as GCC cannot drop a virtual
   * function no one calls; and it put a vtable into RAM, which only a compiler
   * plugin moved to flash. A group object, as after that, still took 2 bytes
   * of RAM, as avr-gcc keeps const data there; the name lies in flash already.
   */
  char const* name_;
};

}  // namespace utl

#endif  // UTL_SRC_UTL_ERROR_GROUP_H_
