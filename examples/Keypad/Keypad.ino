// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A code lock of four keys: buttons on pins 8 to 11, each to ground. A press
// is passed from the pin-change interrupt to loop() through a spsc_queue,
// without turning interrupts off. loop() looks its char up in a table in
// flash, keeps the last four in an inplace_vector, and when they match the
// code, also in flash, opens the lock, the LED on pin 13, through a
// function_ref bound to it; the next key closes it. Serial says what happens,
// at 115200 baud.

#include <Utl.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sfr_defs.h>
#include <pins_arduino.h>
#include <stddef.h>
#include <stdint.h>

namespace {

// Pins 8 to 11 are PB0 to PB3, whose changes raise PCINT0.
constexpr uint8_t kFirstPin{8};
constexpr uint8_t kKeyCount{4};
constexpr uint8_t kKeyMask{0x0f};
// A press within this time of the last one is the bounce of a contact.
constexpr uint16_t kBounceMs{30};

// The char of each key, and the code that opens the lock: in flash, not RAM.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr char kKeyChars[] PROGMEM{'A', 'B', 'C', 'D'};
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr char kCode[] PROGMEM{'C', 'A', 'B', 'A'};
constexpr size_t kCodeLength{sizeof(kCode)};

class Lock {
 public:
  void Open() {
    open_ = true;
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println(F(" -> open"));
  }

  void Close() {
    open_ = false;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("closed"));
  }

  bool IsOpen() const { return open_; }

 private:
  bool open_{false};
};

Lock lock;
// Kept, which a function_ref of a lambda could not be: it refers to the lock
// alone, which lives as long as the sketch.
utl::function_ref<void()> const kOnMatch{UTL_CW(&Lock::Open), lock};

// From the interrupt to loop(): the index of each key pressed.
utl::spsc_queue<uint8_t, 8> presses;
// Presses the queue had no room for: written by the interrupt alone, as each
// count of the queue is by one side, and read by loop(), which keeps its own
// count of those it reported.
uint8_t volatile dropped{0};
uint8_t reported{0};

// The interrupt's own.
uint8_t last_pins{kKeyMask};  // all up, pulled high
uint16_t last_press{0};

// The last keys entered, at most as many as the code has.
utl::inplace_vector<char, kCodeLength> entered;

// Returns whether the keys entered, in RAM, are the code, in flash.
bool Matches(utl::span<char const> keys, utl::flash_span<char> code) {
  if (keys.size() != code.size()) {
    return false;
  }
  for (size_t index{0}; index < keys.size(); ++index) {
    if (keys[index] != code[index]) {
      return false;
    }
  }
  return true;
}

// Adds the key to those entered, dropping the oldest if there are as many
// as the code has, and opens the lock if they match it.
void Enter(char key) {
  if (lock.IsOpen()) {
    lock.Close();
  }
  if (entered.size() == kCodeLength) {
    entered.erase(entered.begin());
  }
  static_cast<void>(entered.push_back(key));  // room was made above
  Serial.print(key);
  if (Matches(entered, utl::flash_span<char>{kCode})) {
    kOnMatch();
    entered.clear();
  }
}

}  // namespace

// A change on pins 8 to 11: a key that went down is pushed, unless it is the
// bounce of the last.
ISR(PCINT0_vect) {
  auto const pins = static_cast<uint8_t>(PINB & kKeyMask);
  // A key is down when its pin is low: up before and down now went down.
  auto const down = static_cast<uint8_t>(pins ^ kKeyMask);
  auto const went_down = static_cast<uint8_t>(last_pins & down);
  last_pins = pins;
  auto const now = static_cast<uint16_t>(millis());
  if (went_down == 0 || now - last_press < kBounceMs) {
    return;
  }
  last_press = now;
  for (uint8_t key{0}; key < kKeyCount; ++key) {
    if ((went_down & (1U << key)) != 0 && !presses.push(key).IsOk()) {
      dropped = static_cast<uint8_t>(dropped + 1);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t key{0}; key < kKeyCount; ++key) {
    pinMode(kFirstPin + key, INPUT_PULLUP);
  }
  PCMSK0 |= kKeyMask;   // pins 8 to 11 raise PCINT0
  PCICR |= _BV(PCIE0);  // and PCINT0 is on
  lock.Close();
}

void loop() {
  utl::flash_span<char> const key_chars{kKeyChars};
  while (utl::optional<uint8_t> const key{presses.pop()}) {
    Enter(key_chars.at(*key).ValueOr('?'));
  }
  uint8_t const lost{dropped};
  if (lost != reported) {
    Serial.println(F(" (keys lost: loop() was too slow)"));
    reported = lost;
  }
}
