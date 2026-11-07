# Utl

A small standard library for Arduino on the AVR. avr-gcc comes without the C++ standard library;
Utl brings what a sketch or a library misses most, in the namespace `utl`: in the manner of the
standard where it has an answer, and checked where the standard leaves a call undefined.

- **Errors** that say what went wrong and where it came from, in 3 bytes each: `Error`, `Status`,
  `Result<T>` and `Result<void>`, with a fluent API after Rust, and groups of errors that any
  library or sketch adds without changing this one.
- **Containers**: `array`, `span` and `make_array`, as in the standard, and `inplace_vector`, as
  in C++26, a vector without the heap, except that what can go out of range or be full returns a
  `Result` or a `Status` rather than being undefined or throwing; and, which the standard does not
  know, `flash_span`, a span of a table in flash, and `spsc_queue`, a queue from an interrupt to
  `loop()` without turning interrupts off.
- **`optional`**, a value or none, as in the standard, except that `value()` of an empty one
  returns a `Result` rather than throwing; also of a reference, `optional<T&>`, as in C++26.
- **`reference_wrapper`**, a reference that copies, as in the standard, for a member that refers to
  an object another owns.
- **`function_ref`**, a reference to something to call, as in C++26, for a parameter that takes a
  function, a lambda or an object without being a template, or for a member function bound to
  its object, given with `UTL_CW()`.
- **`inplace_function`**, something to call kept inside, as `std::function` without the heap, for
  a callback that is kept, a lambda with its captures among them.
- **Text**: `string_view` for text in RAM, as in the standard, and `flash_string_view` for text in
  flash, `PROGMEM` or `F("...")`, which the standard does not know; the two compare with each
  other.
- **Numbers**: `to_chars` and `from_chars`, integers to text and back, as in the standard, with a
  `Result` in place of `std::errc`, and each in the width of its type.

## Installing

Not in the Library Manager yet. Clone it into the `libraries` folder of your sketchbook, under the
name `Utl`:

```bash
git clone https://github.com/theAndreas/Utl ~/Arduino/libraries/Utl
```

It is for the AVR boards (`architectures=avr`); it is built and checked for the Uno.

## Errors

An error is a code within a group, `font: unknown char` or `bus: nack`, and a group is identified
by its address: no one hands out group ids, and a library or a sketch adds a group of its own
without changing this one. It tries out the variant "by its address" of
[MaxMatrix' ErrorHandling.md](https://github.com/theAndreas/MaxMatrix/blob/main/docu/ErrorHandling.md).

### Using them

```cpp
#include <Utl.h>

// A function with nothing else to return returns a Status: ok, or an error.
utl::Status SetChar(char c) {
  if (c < ' ') {
    return FontError::kUnknownChar;  // a code of a group is an error
  }
  return utl::Status::Ok();
}

// A function with a value returns a Result: the value, or an error.
utl::Result<char> Receive(bool acknowledged) {
  if (!acknowledged) {
    return BusError::kNack;
  }
  return 'A';
}

// The first error ends the chain.
utl::Status const status =
    Receive(false).AndThen([](char c) { return SetChar(c); });
if (status == BusError::kNack) { ... }
if (utl::Error const* error = status.error()) {
  Serial.println(error->code());  // 0: kNack, within the group "bus"
}
status.PrintTo(Serial);  // "bus: nack", or "ok"
Serial.println();
```

A sketch includes `<Utl.h>`, and before any header of its own that includes `<utl/...>`:
arduino-cli finds a library only by a header directly in its `src/`, going through the includes
in order, and only once it has found it does `<utl/...>` work.

### Defining a group

A group is its name, a text in flash whose address no other group has; an enum class of its codes;
a `GetErrorGroup()` beside the enum; and, for a group with texts, a table of them and a function
that turns them on. From `examples/Basics`:

```cpp
// font_error.h
enum class FontError : uint8_t {
  kUnknownChar,
  kNullText,
};

extern char const kFontErrorGroup[] PROGMEM;  // "font"; its address is the group

constexpr utl::ErrorGroup GetErrorGroup(FontError) {
  return utl::ErrorGroup(kFontErrorGroup);
}

void UseFontMessages();  // turns the texts on
```

```cpp
// font_error.cpp
char const kFontErrorGroup[] PROGMEM = "font";

char const kUnknownCharText[] PROGMEM = "unknown char";
char const kNullTextText[] PROGMEM = "null text";

// In the order of FontError.
constexpr auto kMessages PROGMEM =
    utl::make_array(kUnknownCharText, kNullTextText);

void UseFontMessages() {
  static utl::Messages messages{utl::ErrorGroup(kFontErrorGroup), kMessages};
  utl::UseMessages(messages);
}
```

- **The name, defined in one `.cpp`**, and declared `extern` in the header. A `const` array
  defined in a header gets a copy in every file that includes it, each at an address of its own,
  and errors made in different files would not compare equal. Two groups may say the same words;
  they are two groups, as their arrays are two.
- **`static` with a constant initializer**: `Messages` names the group by its array, not through
  a function, so that it is set up when compiled; a call in the initializer would make it a static
  initialized at run time, with a guard, which costs flash and RAM.
- **`GetErrorGroup()` lies beside the enum**, in its namespace, where argument-dependent lookup
  finds it. It is what lets a code convert to an `Error`; an enum without one does not.
- **The texts lie in a `constexpr` table in `PROGMEM`.** `constexpr`, so that the table is
  complete when compiled: one initialized at start-up would be written to flash, which fails
  without a word. A group has at most 255 texts, as a code is a `uint8_t`; `Messages` checks that
  when compiled.
- **Codes of different groups may overlap**: two errors are equal only if group and code are.

### The types

One class per file in `src/utl/`:

- `Error` (`error.h`): always an error, a group and a code; there is no `Error()`. A code of a
  group converts to it implicitly, so `Error e{FontError::kUnknownChar}` and
  `error == FontError::kUnknownChar` need no function around the code; no other type converts.
- `Status` (`status.h`): what a function returns that has nothing else to return, ok or an
  `Error`, as Rust's `Result<(), E>` or `absl::Status`. `Status::Ok()` is ok, a code or an `Error`
  converts to a `Status`, and `error()` gives the `Error` or null, so that no one gets hold
  of an `Error` that is none. Still 3 bytes: inside, ok is an `Error` with a null group, which
  only `Status` can make.
- `Result<T>` (`result.h`): a value or an `Error`, the value and a `Status` side by side rather
  than in a union; `error()` as in `Status`, and `status()` to compare or print.
- `Result<void>` (`result.h`): a `Status`, for code that is generic in `T`. It is one, by
  inheritance, and converts back; `Map()` with a function that returns nothing gives one, and
  its own `Map()` makes a value from "ok". A sketch's function returns a `Status`.
- `Result<T&>` (`result.h`): a reference to an object that lies elsewhere, or an `Error`, for a
  function that finds an object or fails, `Result<Led&> GetLed(uint8_t pin)`. A pointer and a
  `Status`, 5 bytes; `T` needs no default constructor. It binds an lvalue only, assigning one
  rebinds it, and `Map()` and the others give their function the `T&` itself. `value()` of one
  with an error is undefined, as there is no `T` to give; `ValueOr()` gives a `T&`, of the object
  or of the fallback.
- `ErrorGroup` (`error_group.h`): a group, the address of its name in flash, a value of 2 bytes that
  takes no memory of its own; no virtual function.
- `Messages` (`messages.h`): the texts of a group, and `UseMessages()`, which turns them on.

The fluent functions follow Rust's `Result`, on `Status` and `Result`: `AndThen()` goes on while
there is no error, `Map()` changes the value, `OrElse()` recovers from an error and gets an
`Error` that is one, `ValueOr()` gives a fallback. None calls its function when it has nothing
to give it. A chain of three steps costs 14 bytes of flash more than the same `if`s written out,
and no RAM: the lambdas are inlined.

More of Rust's, under its names where Utl has a word of its own for them:

- `ValueOrElse(f)`, Rust's `unwrap_or_else()`: the value, or a fallback that `f` makes from the
  error, only when it is needed.
- `Inspect(f)` and `InspectError(f)`, Rust's `inspect()` and `inspect_err()`: look at the value or
  the error on its way, to print it, and pass the `Result` on as it is. `f` gets a `T const&`, as
  it looks and does not change.
- `MapError(f)`, Rust's `map_err()`: an error of a lower layer said in a group of one's own. `f`
  returns an `Error` or a code, never ok; that is what `OrElse()` is for.
- `ConsumeError(f)`, only on a `Status` and so on a `Result<void>`: calls `f` with the error, if
  there is one, and returns nothing, the end of a chain. Rust ends one with `if let` or `match`;
  a `Result<T>` has none, as it would drop the value unseen.

What the function may return is checked when compiled, with a message that says what to use
instead. A function that only looks, for `Inspect()`, `InspectError()` and `ConsumeError()`,
returns nothing, as in Rust: one that returns a `Status`, or anything else, would have it dropped
unseen. `AndThen()` wants a `Result`, a `Status` or an `optional` back, and points to `Map()` for
a plain value; `MapError()` wants an `Error` or a code, and points to `OrElse()` to recover.

```cpp
Receive(true)
    .Map([](char c) { Serial.println(c); })                         // ok: a Result<void>
    .ConsumeError([](utl::Error const& e) { e.PrintTo(Serial); });  // or the error, and done
```

`Map()` with a function that returns nothing gives a `Result<void>`, which is a `Status`:

```cpp
utl::Result<void> const printed = Receive(true).Map([](char c) { Serial.println(c); });
```

An `Error` and a `Result<T>` are `[[nodiscard]]`: a call that returns a value and has it dropped
warns, as `[[nodiscard]]` on a type, from C++17, which avr-gcc 7 takes under gnu++11 as well. A
`Status` is not, nor `Result<void>`: a sketch drops the status of `SetText("Hi!")` as a matter of
course, and with the attribute every such call warned. A library marks a function whose status
must be looked at itself, `[[nodiscard]] utl::Status Begin();`. `static_cast<void>(Get())` drops
a value on purpose. The Arduino IDE shows the warning once "Compiler warnings" is set above "None".

### Texts

A sketch that wants an error in words turns its group's texts on, once, usually in `setup()`:

```cpp
bus::UseBusMessages();  // "bus: nack" from now on; before, "bus: 0"
```

Until then an error prints its code, `font: 0`, and `Message()` is empty; so does a code that its
group has no text for. Only a sketch that calls
a group's function links its texts, as nothing else names them: a sketch that never prints an
error, or prints only codes, pays for no text, and one that prints some groups in words pays for
those. A switch for the whole build, `UTL_ERROR_TEXTS` before, could not tell them apart, and the
Arduino IDE sets no switch of a library without `platform.local.txt`.

Before, `Name()` and `Message()` were virtual. GCC cannot drop a virtual function no one calls, so
every text of every group was linked into any sketch that made an error, printed or not; and each
group had a vtable in RAM, which only the plugin of [vtables in flash](#vtables-in-flash) moved.
Now a group is the address of its name, which lies in flash anyway, and the texts of the groups
that are on are a list through their `Messages` objects, 7 bytes of RAM each, in the sketch that
turns them on.

On the Uno, with `Basics` printing both of its groups in words: 2452 bytes of flash and 204 of RAM,
against 2376 and 208 with virtual groups, or 2378 and 174 with those and the plugin. The 76 bytes
are the search through the list and the printing of a code, which a sketch that prints an error
links whether it needs it or not. A sketch that makes errors and prints none links neither, nor
any text.

## Containers

```cpp
utl::array<uint8_t, 3> rows = {{0x18, 0x3c, 0x18}};
for (uint8_t const row : rows) { ... }
rows.at(5).ValueOr(0);  // 0, as 5 lies beyond the end: at() returns a Result

// A span takes a C array, a utl::array and another span alike.
uint16_t Sum(utl::span<uint8_t const> values);
Sum(rows);
utl::span<uint8_t>(rows).subspan(1).Map(Sum);  // a Result: subspan() can fail

constexpr auto kPins = utl::make_array(uint8_t{2}, uint8_t{3});  // utl::array<uint8_t, 2>
```

- `array<T, N>` (`array.h`) and `span<T>` (`span.h`): `std::array`, which avr-gcc lacks, and
  `std::span` from C++20, with the STL names Google allows for an STL-like entity. What the
  standard leaves undefined or throws on beyond the end returns a `Result` with
  `IndexError::kOutOfRange` instead: `at()`, and `first()`, `last()`, `subspan()` of a span.
  `operator[]` stays unchecked, unless [checked](#checked-access). The group `IndexError` costs
  its 2 bytes only in a sketch that calls one of these, and its text only in one that calls
  `utl::UseIndexMessages()`.
- `make_array(...)` (`array.h`): an array whose type and size come from its arguments, what
  `std::array a = {...}` deduces from C++17 on. It refuses arguments that would narrow, although
  Arduino compiles with `-fpermissive -Wno-error=narrowing`.

### inplace_vector

```cpp
utl::inplace_vector<uint8_t, 8> pressed;      // empty, room for 8
pressed.push_back(3);                        // a Status: ok, or IndexError::kFull
utl::inplace_vector<uint8_t, 8> pins{2, 3, 4};
pins.insert(pins.begin(), 1);                // 1, 2, 3, 4
pins.erase(pins.begin() + 1);                // 1, 3, 4
Sum(pins);                                   // a span<uint8_t const> of 3
```

`inplace_vector<T, N>` (`inplace_vector.h`) is C++26's `std::inplace_vector`: a vector of at most
`N` elements, kept inside, never on the heap. It is the room for `N` elements and a count, 1 byte
for `N` up to 255: an `inplace_vector<uint8_t, 8>` is 9 bytes. Elements are made when added and
destroyed when removed, so `T` needs no default constructor, and one of elements that are trivially
copyable is copied as its bytes. What adds an element, `push_back()`, `emplace_back()`,
`insert()`, `emplace()` and `resize()`, returns a `Status` with `IndexError::kFull` where the
standard throws `std::bad_alloc`: not a reference or an iterator, as a `Result` would warn
wherever a sketch drops it, and a `Status` does not. `insert()` puts the element at the position
given, or returns `IndexError::kOutOfRange` for one outside. `at()` returns a `Result`, as of an
array, and a span takes an `inplace_vector` as it takes an array. `pop_back()` of an empty one and
`erase()` beyond the end do nothing, [checked](#checked-access), where the standard leaves them
undefined.

One made empty is a constant: a global one lies in `.bss` and costs no code to set up. One made of
a braced list, `{2, 3, 4}`, is set up when the sketch starts, with the list in RAM besides; a table
that never changes is cheaper as a `constexpr` array. A braced list needs `std::initializer_list`,
which avr-gcc lacks: Utl defines it where no `<initializer_list>` is found, laid out as GCC's own and
under its include guard, so that a library that brings the header keeps its own. A list of more
than `N` keeps the first `N`, [checked](#checked-access); a constant that does not fit `T` narrows,
as in the standard, which Arduino's flags let pass with a warning.

Left out are the `try_` and `unchecked_` variants, which `push_back()` covers; a constructor of `n`
elements, which `resize()` does; and `reserve()`, `shrink_to_fit()`, `swap()`, `assign()`,
`append_range()` and `<`.

### flash_span

```cpp
constexpr uint8_t kSmiley[] PROGMEM = {0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c};

utl::flash_span<uint8_t> const smiley(kSmiley);  // explicit: the array lies in flash
for (uint8_t const row : smiley) { ... }         // each row read from flash
smiley.at(9).ValueOr(0);                         // 0, as 9 lies beyond the end
smiley.subspan(2, 4).Map(Draw);                  // a Result: subspan() can fail
```

`flash_span<T>` (`flash_span.h`) is a `span` of elements in flash, `PROGMEM`, as
`flash_string_view` is a `string_view` of text there: a table that never changes, a font or a
pattern, lies in flash rather than in RAM, where the Uno has 2 KB. It is a pointer and a size, 4
bytes. Each element is read with `pgm_read_byte()`, `pgm_read_word()` or `pgm_read_dword()` by its
size, or `memcpy_P()` for any other, and given as a value, not a reference, as there is nothing in
RAM to refer to; so `T` is trivially copyable, and not `const`, as nothing writes to flash anyway.
Its names are those of `span`, and `at()`, `first()`, `last()` and `subspan()` return a `Result`
as there; `[]`, `front()` and `back()` are [checked](#checked-access).

It is made of a C array or a `utl::array` only explicitly, `flash_span<uint8_t>(kSmiley)`, as the
type of an array does not tell where it lies: one of an array in RAM reads whatever lies in flash
at that address, and nothing stops it. Flash beyond 64 KB, which `pgm_read_byte_far()` reads on a
Mega, is left out, as Utl is for the Uno.

### spsc_queue

```cpp
utl::spsc_queue<uint8_t, 16> received;           // 16 bytes and two counts, 18 bytes

ISR(USART_RX_vect) { received.push(UDR0); }      // a Status: ok, or IndexError::kFull

void loop() {
  while (utl::optional<uint8_t> const byte = received.pop()) {
    Handle(*byte);
  }
}
```

`spsc_queue<T, N>` (`spsc_queue.h`) carries elements from one producer to one consumer, such as an
interrupt and `loop()`, in the order added, without turning interrupts off. Each side writes a
count of its own, a byte, which the other side reads at once on the AVR: `push()` writes the
element, then its count, `pop()` reads the element, then its count, and `__atomic_signal_fence()`
keeps the compiler to that order, with no instruction of its own. The counts run on and wrap, and
an element's place is its count masked: so `N` is a power of two from 1 to 128, and all `N` places
are used. In the interrupt `push()` is a test, a store and a count, with no `cli()`.

`push()` takes the element by value, so that a register passes as it is, and returns a `Status`
with `IndexError::kFull` when full, which leaves it out; `pop()` and `front()` return an `optional`,
as empty is no error. `size()`, `empty()` and `full()` say what is now, which the other side may
change at once. `T` is trivially copyable, so that copying it in an interrupt runs no code of its
own, and has a default constructor, for the `optional`. One producer and one consumer, on one
core: two producers, or threads on cores of their own, need more than this. A global one lies in
`.bss` and costs no code to set up.

## optional

```cpp
utl::optional<uint8_t> FindRow(uint8_t value);  // returns utl::nullopt if there is none

if (utl::optional<uint8_t> const row = FindRow(0x3c)) { Use(*row); }
FindRow(0x3c).ValueOr(0);
FindRow(0x3c).value();                         // a Result: OptionalError::kEmpty if none
FindRow(0x3c).OkOr(PanelError::kNoSuchRow);  // a Result, with an error of your own
```

`optional<T>` (`optional.h`) is `std::optional`, for "none" that is no error; a function that can
fail returns a `Result`. It keeps the standard's names, except where `Result` does the same:
`HasValue()`, `ValueOr()`, `OkOr()`, `AndThen()`, `Map()` and `OrElse()` are named as there, so
that a value or none and a value or an error read alike. The value and the flag lie side by
side, as in a `Result`, so `T` needs a default constructor; an `optional<uint8_t>` is 2 bytes.
`value()` cannot throw `bad_optional_access`, so it returns a `Result` with
`OptionalError::kEmpty`, the group `optional`; `OkOr()`, as `ok_or()` in Rust, gives your own
error instead. `AndThen()`, `Map()` and `OrElse()` are what C++23's `std::optional` calls
`and_then()`, `transform()` and `or_else()`. From Rust's `Option` it has `ValueOrElse(f)`, a
fallback made only when it is needed, `Inspect(f)`, and `Filter(predicate)`, which empties it
unless the predicate holds.

```cpp
utl::optional<Led&> FindLed(uint8_t pin);  // a reference, or utl::nullopt

if (utl::optional<Led&> const led = FindLed(13)) { led->On(); }
FindLed(13).ValueOr(spare).On();  // the LED, or the spare itself, not a copy
```

`optional<T&>` is C++26's `std::optional<T&>`: a reference to an object that lies elsewhere, or
none. It is a pointer, 2 bytes, null if empty, and `T` needs no default constructor. It binds an
lvalue only, never a temporary; assigning one rebinds it, as in C++26, and never writes through
it. `*` and `->` of an empty one are undefined, as there is no `T` to give, and
[checked](#checked-access) like those of an `optional<T>`. `value()` gives a `Result<T&>`. One
change: `ValueOr()` gives a `T&`, of the object or of the fallback, which has to be an lvalue for
that reason, where C++26's `value_or()` gives a copy, which would switch on a copy of the LED
above. `ValueOrElse(f)` gives a `T&` as well, and `f` has to return one, of an object that
outlives the call; a `Result<T&>` does the same. A reference that is never absent, a member above
all, is a [`reference_wrapper`](#reference_wrapper).

## reference_wrapper

```cpp
class Panel {
 public:
  explicit Panel(Display& display) : display_(display) {}
  void Clear() { display_.get().Clear(); }

 private:
  utl::reference_wrapper<Display> display_;  // not Display&, not Display*
};

utl::array<utl::reference_wrapper<Led>, 2> leds = {{red, green}};
auto const read_only = utl::cref(red);  // a utl::reference_wrapper<Led const>
```

`reference_wrapper<T>` (`reference_wrapper.h`) is `std::reference_wrapper`, with `get()`, the
conversion to `T&`, `ref()` and `cref()`. It is a pointer, 2 bytes, that is never null and owns
nothing: a member that is one leaves its class copyable and assignable, as a `T&` would not, and
says what a `T*` leaves open, that it is never null and no one deletes it. It binds an lvalue only,
never a temporary, and there is no `reference_wrapper()`; assigning one rebinds it, as in the
standard. It converts as a pointer does, to one of `T const` or of a base class. Left out is
`operator()`, which calls a function it refers to as `std::invoke`: the one costly part, and
nothing a member needs.

## function_ref

```cpp
void ForEachLed(utl::function_ref<void(Led&)> f) {  // no template, and no heap
  for (Led& led : leds) {
    f(led);
  }
}

uint8_t on = 0;
ForEachLed([&on](Led& led) { on += led.IsOn(); });  // a lambda with its captures
ForEachLed(SwitchOff);                              // or a function
```

`function_ref<R(Args...)>` (`function_ref.h`) is C++26's `std::function_ref`: it refers to a
function, a lambda or an object with `operator()`, and calls it with `Args`, its result converted
to an `R`, or dropped for `void`. It is 4 bytes, a pointer to what it refers to and one to the code
that calls it, and owns nothing: no heap, no virtual function, and a function that takes one is no
template. It is meant for a parameter. It never copies a lambda, so the lambda has to outlive every
call: written as the argument, it lives until the call returns, but kept in a member it is gone
before the next call. One of a function, `= SwitchOff`, may be kept. There is no
`function_ref()`, so it is never empty; assigning a lambda is deleted, assigning another
`function_ref` or a function rebinds it. A null function pointer is [checked](#checked-access).
Left out are the signatures with `const` and `noexcept`, which gnu++11 cannot have.

```cpp
class Button {
 public:
  explicit Button(utl::function_ref<void()> on_press) : on_press_(on_press) {}

 private:
  utl::function_ref<void()> on_press_;  // kept: it refers to the LED, not to a lambda
};

Button button{{UTL_CW(&Led::Toggle), led}};  // Toggle() of led
ForEachLed(UTL_CW(&Led::Toggle));            // Toggle() of each LED, the argument of the call
```

A member function is bound to its object with `UTL_CW()`, where C++26 writes `std::cw<...>`:
`{UTL_CW(&Led::Toggle), led}` refers to `led` and calls its `Toggle()`, `{UTL_CW(&Led::Toggle),
&led}` keeps the pointer, and `UTL_CW(&Led::Toggle)` alone calls `Toggle()` of the first argument.
The function is a template argument, part of the code that calls it, so the `function_ref` is
still 4 bytes and refers to the object alone: unlike one of a lambda, it may be kept as long as the
object lives. It binds an lvalue, never a temporary, and calls a `const` member function of a
`const` object; a function that takes the object first binds as a member function does. A null
pointer to the object of a member function is [checked](#checked-access), a null function stops
the build.

`UTL_CW(value)` gives a `constant_wrapper<T, value>` (`constant_wrapper.h`), C++26's
`std::constant_wrapper`, a value carried in a type. C++26 deduces `T` from the value with
`template <auto>`, which gnu++11 lacks: so the type comes first, and the macro names it with
`decltype`. It holds `value`, `value_type` and `type`, and none of the standard's operators.

## inplace_function

```cpp
class Button {
 public:
  void OnPress(utl::inplace_function<void()> const& handler) { on_press_ = handler; }
  void Poll() {
    if (Pressed() && on_press_) {
      on_press_();
    }
  }

 private:
  utl::inplace_function<void()> on_press_;  // empty: nothing to call
};

button.OnPress([this] { Toggle(); });  // a copy of the lambda, kept inside
```

`inplace_function<R(Args...), Capacity>` (`inplace_function.h`) is `std::function` without the
heap, as SG14's `inplace_function` is: it keeps a copy of a function, a lambda with its captures or
an object with `operator()` in `Capacity` bytes of its own, by default two pointers, 4 bytes on
the Uno, room for `[this]` and a byte. A callable larger than that stops the build. It is
`Capacity` and 4 bytes, 8 by default: two pointers to code, one to call and one to copy and
destroy, which is null for a callable that is trivially copyable, as a lambda that captures
pointers, references or numbers, copied as its bytes. Where a `function_ref` refers to a lambda
that is gone after the call, it keeps one; it is copied and assigned as a value, and copies its
callable with it. It is empty when made without a callable, from `nullptr` or a null function
pointer, and after it was moved from; `operator bool` asks. A call of an empty one does nothing
and returns `R()`, [checked](#checked-access), where `std::function` throws; so `R` is `void` or
has a default constructor, and is never a reference. Left out are `swap()`, the comparisons with
`nullptr` and `target()`, which needs RTTI.

## Text: string_view and flash_string_view

```cpp
// A line typed on Serial, in RAM, against keywords in flash.
utl::Status Execute(utl::string_view line) {
  if (line == F("clear")) {
    return Clear();
  }
  if (line.starts_with(F("text="))) {
    return line.substr(5).AndThen(SetText);  // the text after "text="
  }
  return CommandError::kUnknown;
}

line.find('=');  // 4 in "text=Hi", or utl::string_view::npos

utl::flash_string_view const keyword = F("text=");
for (char const c : keyword) { ... }  // each char read from flash
```

`string_view` (`string_view.h`) is `std::string_view` for text in RAM, with its names; `at()`
and `substr()` return a `Result` where the standard throws, and `find()` returns `npos`, as "not
found" is no error. `flash_string_view` (`flash_string_view.h`) has the same names for text in
flash. On the AVR flash is an address space of its own, and a pointer into it read as one into
RAM gives whatever lies at that address in RAM; so it reads every char with `pgm_read_byte()`,
takes the type `F()` gives rather than a `char const*`, and its `operator[]` returns a char, not a
reference. Both are 4 bytes, a pointer and a size, and own nothing. `string_view` also has
`remove_prefix()`, `remove_suffix()`, `compare()` and `<`, which orders as `unsigned char`, as the
standard does; `compare()` gives an `int8_t`, -1, 0 or 1, not an `int`. `starts_with()`,
`ends_with()`, `find()` and `==` work between RAM and flash.

## Numbers: to_chars and from_chars

```cpp
// "speed=42" typed on Serial: the number after the "=".
utl::string_view rest = line.substr(6).ValueOr("");
utl::Result<uint8_t> const speed = utl::from_chars<uint8_t>(rest);  // rest is "" now

// A number on a display, without Print.
utl::array<char, 6> buffer;
utl::to_chars(buffer, int16_t{-273}).Map(Show);  // "-273"
```

`to_chars()` and `from_chars()` (`charconv.h`) are `std::to_chars` and `std::from_chars` for
integers: a minus for a signed type, digits beyond 9 in either case when read and lower case when
written, bases 2 to 36. They read strictly, as the standard does: no plus, no space, no `0x`. Each
returns a `Result`, the end of what it wrote or read, or a `CharconvError` of the group
`charconv`: `kInvalidArgument`, no number where one should begin or a base not 2..36;
`kOutOfRange`, a number too large for its type; `kValueTooLarge`, a text that does not fit the
buffer. `utl::UseCharconvMessages()` turns their texts on. Beside the standard's pointer pairs,
`to_chars()` writes into a `span<char>` and gives the text as a `string_view`, and
`from_chars<T>()` reads from the front of a `string_view` and leaves it behind the number.

A number is converted in the width of its type: `to_chars()` of a `uint8_t` links an 8-bit
division only, where `Serial.print()` of one links the 32-bit division of an `unsigned long`. A
sketch that prints a `uint8_t` with it takes 1574 bytes of flash, against 1616 with
`Serial.print()`, and the same RAM. Three changes: the base is a `uint8_t`, not an `int`; an
error gives no pointer, as a `Result` holds a value or an error, where the standard gives one past
the digits of a number out of range; and there is no floating point, as a correct `to_chars()` of
a `double` would not fit on an Uno, and avr-libc has `dtostrf()`.

## Checked access

What the standard leaves undefined, or what gives a placeholder here, can be reported before it
happens, for finding a mistake while a sketch is written, as the DET of AUTOSAR Classic does:
`value()` of a `Result` with an error, `*` and `->` of an empty `optional`, `[]` beyond the end,
`front()` or `back()` of an empty `span`, `flash_span`, `string_view` or `inplace_vector`,
`pop_back()` of an empty `inplace_vector`, `erase()` beyond its end and one made of more elements
than it holds, which then each do nothing or keep what fits, and `remove_prefix()` or
`remove_suffix()` of more than there is, which then leaves an empty view, a `function_ref` made of a
null function pointer, or of a member function and a null pointer to its object, and a call of an
empty `inplace_function`, which returns `R()`. With `UTL_CHECK_ACCESS` at 1, set for the whole build
as `config.h` says, each goes to `utl::ReportMisuse()` first, which prints `utl: [] beyond the end`
on `Serial`:

```cpp
// A report of your own, in a .cpp of the sketch, replaces the default.
namespace utl {
void ReportMisuse(Misuse) {
  digitalWrite(LED_BUILTIN, HIGH);  // stays lit: something was misused
}
}  // namespace utl
```

At 0, the default, nothing of it is compiled: every example is the same to the byte as without
it. The default report lies in a file of its own and the library is linked as an archive
(`dot_a_linkage`), as in MaxMatrix: a sketch with a report of its own pulls in no `Serial`, 13
bytes of RAM against 190 with the default. A report of your own belongs in a `.cpp` of the
sketch, not the `.ino`, for which arduino-cli writes prototypes outside the namespace.

## Examples

- `Basics`: two groups, a `Status` and a `Result` chained, and the error printed on `Serial`: the
  bus in words, the font as a code, as only the bus's texts are turned on.
- `Keypad`: a code lock of four buttons on pins 8 to 11. The pin-change interrupt passes each press
  to `loop()` through a `spsc_queue`, without `cli()`; `loop()` looks its char up in a table in
  flash, a `flash_span`, keeps the last four in an `inplace_vector`, and when they match the code,
  also in flash, opens the lock through a `function_ref` bound to it with `UTL_CW()`. 2484 bytes of
  flash and 212 of RAM, little more than `Basics`.
- `SelfCheck`: checks the library and prints a line per check, then the number that failed.
  `tools/self_check.sh` runs it in simavr and compares with `SelfCheck.expected`. Its checks lie
  in one file per area, `check_*.cpp`; a new area is a file there and a line in `setup()`.

## vtables in flash

utl has no virtual function and no vtable of its own. The Arduino core has: `HardwareSerial`,
whose vtable avr-gcc keeps in RAM, as it reads data with `LD` from the data space and the linker
copies all of `.rodata` into RAM at start-up.

The GCC plugin [avr-flash-vtbl](https://github.com/jcmvbkbc/avr-flash-vtbl) puts the vtables into
flash instead: it marks the vtable types `__flash`, and a virtual call then reads its function
pointer with `LPM`. A copy of it, at upstream commit `fbc6ed9`, lies in `tools/avr-flash-vtbl/`
under its own licence, the GPL v2; it is a tool of the build and links nothing into a sketch.
utl needed it while its groups were virtual; it now saves only the core's 18 bytes, and the
examples are built with it to show that utl does not depend on it.

| Example | Flash, ram | Flash, flash | Flash, checked | RAM, ram | RAM, flash | RAM, checked |
|---|---|---|---|---|---|---|
| Basics | 2404 | 2404 | 2404 | 196 | 178 | 196 |
| Keypad | 2484 | 2484 | 2802 | 212 | 194 | 212 |
| SelfCheck | 20586 | 20586 | 23552 | 354 | 336 | 368 |

`ram` keeps the vtables in RAM, the default of avr-gcc; `flash` puts them into flash with the
plugin; `checked` is `ram` with `UTL_CHECK_ACCESS` at 1. `SelfCheck` checks more in `checked`,
which is most of what it adds there. `arduino:avr:uno`, Arduino AVR Boards 1.8.8 (avr-gcc 7.3.0).

**Every C++ file of a sketch has to be compiled with the plugin**, the Arduino core included. A
file compiled without it reads a vtable in flash from RAM and jumps anywhere; the linker does not
notice. `tools/compile_examples.sh` therefore fails when a vtable of the flash build lies in RAM.

The Arduino IDE passes no flags per sketch. To use the plugin there, add to the
`platform.local.txt` beside the `platform.txt` of the AVR core:

```text
compiler.cpp.extra_flags=-fplugin=/full/path/to/avr-flash-vtbl.so
```

A plugin loads only into the `cc1plus` it was built for: build it again after an update of the AVR
core.

## Build and check

Needs `arduino-cli` with `arduino:avr`, a host `g++` for the plugin, `simavr`, `uv` and `jq`; for
the tests on the host CMake, Ninja and GoogleTest (`libgtest-dev`); for the reference Doxygen. The
dev container below has them all.

```bash
tools/build_plugin.sh             # build/avr-flash-vtbl.so
tools/self_check.sh               # examples/SelfCheck in simavr, in each variant
tools/keypad_check.sh             # examples/Keypad in simavr with keys pressed, ~40 s a variant
tools/compile_examples.sh         # every example in each variant, fails on a warning; the table
cmake --workflow --preset default   # the tests on the host, in three variants
cmake --workflow --preset sanitize  # the same under AddressSanitizer and UBSan
tools/coverage.sh                 # the same with --coverage: what of src/ they reach
tools/lint.sh [--fix]             # clang-format, cpplint, shellcheck, actionlint; --fix formats C++
tools/tidy.sh                     # clang-tidy: library and examples as for the Uno, and the tests
tools/docs.sh                     # the reference from the Doxygen blocks, in build/docs/html
```

The dev container in `.devcontainer/` carries all of it, with clangd for IntelliSense, which
`.clangd` points at the compilation databases of CMake and of `tools/tidy.sh`; see
[its README](.devcontainer/README.md).

The tests on the host (`test/`) run the library against a mock of the Arduino core
(`test/mock/`), built as gnu++11 as avr-gcc builds it, as built by default and with misuse
checked, as `SelfCheck` is. They reach every line and every branch of `src/`; gcov counts a
template per instance, so the report merges them per line. What they cannot see is the AVR itself:
the host has no separate flash, so a text in flash read as one in RAM passes there. That, and the
core's vtables in flash, is what `SelfCheck` in simavr is for.

What must not compile has tests too. Each file in `test/compile_fail/` uses utl wrongly, an
`Inspect()` whose function returns a value, say, and names in a line `// Expected: ...` the
message of the `static_assert` that has to stop it. CMake makes a test of each, which calls the
compiler on the file and passes only if that message comes; a `static_assert` that went missing
fails it. A new case is a new file there, nothing else.

CI (`.github/workflows/ci.yml`) runs the same scripts on every pull request, but `keypad_check.sh`,
and `arduino-lint` on top; the table of flash and RAM lands in the summary of the job "Examples on
arduino:avr:uno". `keypad_check.sh` runs apart, in `.github/workflows/keypad.yml`, for a pull
request that changes the sketch or what of the library it uses, and by hand ("Run workflow"), as
simavr takes about 40 s a variant for its 0.3 s of keys.

The reference, built from the Doxygen blocks of `src/`, is published at
<https://theandreas.github.io/Utl/>; how a block is written, and how it is checked, is in
[docs/Documentation.md](docs/Documentation.md). `tools/docs.sh` builds it and fails on a block
Doxygen cannot read or an entity of the API without one; CI runs it on every pull request
(`.github/workflows/docs.yml`) and publishes it from `main`.

The code follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html),
the scripts the [Shell Style Guide](https://google.github.io/styleguide/shellguide.html).
Three exceptions are forced by Arduino: sources end in `.cpp`, as `arduino-cli` compiles no
`.cc`, and so do the tests on the host, which it never sees, so that one ending holds throughout;
the header `Utl.h` is named after the library, as Arduino expects; and a sketch
includes its own headers without a directory (see `examples/CPPLINT.cfg`). One is chosen:
`const` stands right of what it qualifies, `char const*` and `Status const&`, as in MaxMatrix.
The guide prefers it on the left but does not require it; `.clang-format` moves it, and
`tools/lint.sh` fails on one on the left.

`tools/lint.sh` and `tools/tidy.sh` fetch their tools from PyPI through `uvx`, at fixed versions, so
that nothing has to be installed. `.clang-tidy` turns on broad groups of checks and turns off, each
with its reason, what contradicts the Google style or cannot be had on the AVR. It also checks Google's names
with `readability-identifier-naming`, a private member ending in `_`, a parameter in `snake_case`,
a constant with `k`, apart from what mimics the standard library and the accessors, which Google
names like variables: clang-format cannot rename, and cpplint does not look at names.

Nothing has run on a board yet; `SelfCheck` should print the same there as in simavr.

## License

GPL-3.0-or-later, see `LICENSE`. The plugin in `tools/avr-flash-vtbl/` is under its own licence,
the GPL v2, in its `COPYING`.
