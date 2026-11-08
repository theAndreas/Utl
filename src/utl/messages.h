// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The texts of a group's codes, for a sketch that wants its errors in
 *        words.
 */

#ifndef UTL_SRC_UTL_MESSAGES_H_
#define UTL_SRC_UTL_MESSAGES_H_

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "utl/array.h"
#include "utl/error_group.h"

namespace utl {

/*!
 * \brief The texts of one group, which UseMessages() turns on.
 *
 * A group offers a function that makes its Messages and turns them on, beside
 * its table:
 *
 * \code{.cpp}
 * // font_error.cpp
 * char const kUnknownCharText[] PROGMEM = "unknown char";
 * constexpr auto kMessages PROGMEM = utl::make_array(kUnknownCharText);
 *
 * void UseFontMessages() {
 *   static utl::Messages messages{utl::ErrorGroup{kFontErrorGroup},
 *                                 kMessages};
 *   utl::UseMessages(messages);
 * }
 * \endcode
 *
 * The table is linked only into a sketch that calls `UseFontMessages()`, as
 * nothing else names it: a sketch pays for the texts it asks for, and for no
 * others. The object takes 7 bytes of RAM, in the sketch that calls it.
 *
 * \note Name the group by its array, as above, not through a function: the
 *       static has to be initialized when compiled, or it costs a guard.
 */
class Messages {
 public:
  /*!
   * \brief Makes the texts of `group` from its table.
   *
   * \tparam N The number of texts, at most 255, as a code is a `uint8_t`;
   *           checked when compiled.
   * \param[in] group The group the texts belong to.
   * \param[in] texts The table, in `PROGMEM`, a text per code in the order of
   *                  the codes.
   */
  template <size_t N>
  constexpr Messages(ErrorGroup group, array<char const*, N> const& texts)
      : group_{group}, texts_{texts.data()}, count_{static_cast<uint8_t>(N)} {
    // Not UINT8_MAX: avr-libc defines it in C++ only with __STDC_LIMIT_MACROS.
    static_assert(static_cast<uint8_t>(N) == N,
                  "a code is a uint8_t: a group has at most 255 texts");
  }

  /*! \brief Not copyable: UseMessages() keeps its address. */
  Messages(Messages const&) = delete;

  /*! \brief Not assignable: UseMessages() keeps its address. */
  Messages& operator=(Messages const&) = delete;

  /*!
   * \brief Returns the text of `code` of `group`, from flash.
   *
   * \param[in] group The group.
   * \param[in] code The code within the group.
   * \return The text, or null if the texts of the group are off or have none
   *         for the code. Error then prints the code, which says more than
   *         "unknown code" would, and costs no text.
   */
  static __FlashStringHelper const* Find(ErrorGroup group, uint8_t code);

 private:
  friend void UseMessages(Messages& messages);

  /*
   * A Messages per group, linked only into a sketch that calls the group's
   * function: a switch for the whole build, UTL_ERROR_TEXTS before, could not
   * tell the texts a sketch asks for from the others.
   */
  ErrorGroup group_;
  char const* const* texts_;
  uint8_t count_;
  // The next group whose texts are on, a list through the objects that
  // UseMessages() was given, so that it takes no memory of its own.
  Messages* next_{nullptr};
};

/*!
 * \brief Turns on the texts of the group of `messages`.
 *
 * From then on Error::Message() and Error::PrintTo() find them. Once is
 * enough; a second call with the same object does nothing.
 *
 * \param[in,out] messages The texts. They have to live as long as the program,
 *                         as a `static` does.
 */
void UseMessages(Messages& messages);

}  // namespace utl

#endif  // UTL_SRC_UTL_MESSAGES_H_
