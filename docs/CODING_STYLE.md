# FriendlyBot Coding Style Guide

This document defines the coding style and conventions for the **Two-Eyed Bot / FriendlyBot** firmware project. All code must adhere to these conventions and match the repository's `.clang-format` and `.clang-tidy` rules.

---

## 1. Language Standard & General Rules

- **Standard**: C++20 (`-std=gnu++2a` / `-std=c++2a`).
- **Target Platform**: ESP32-S3 Arduino Framework via PlatformIO.
- **Resource Discipline**: Use stack allocation, static storage, or RAII containers. Avoid raw `new`/`delete` and unchecked dynamic memory allocation.

---

## 2. Formatting & Layout

### 2.1 Pointers and References
- **Pointers**: Right-aligned (`Type *ptr`, `char *buf`).
- **References**: Left-aligned (`Type& ref`, `const std::string& str`).

```cpp
void processData( const DataPacket& packet, uint8_t *outputBuffer )
{
    // ...
}
```

### 2.2 Parentheses & Brackets Spacing
- Add spaces **inside** all parentheses (conditions, expressions, function calls, casts):
  ```cpp
  if( condition ) {
      doSomething( a, b );
      ...
  } else {
      doSomethingElse( c, d );
      ...
  }
  ```
  Omit braces for single-statements blocks, when unambiguous:
  ```cpp
  if( condition )
      doSomething( a, b );
  else
      doSomethingElse( c, d );
  ```
- Add spaces **inside** square brackets:
  ```cpp
  const auto val = buffer[ index ];
  ```
- **No space** between function name and opening parenthesis:
  ```cpp
  int calculateTime( int minutes ); // correct
  ```

### 2.3 Bracing & Block Indentation
- Indentation is **4 spaces** (no tabs).
- Opening braces `{` are placed on a **new line** for classes, structs, functions, namespaces, and control flow blocks (`if`, `else`, `while`, `for`, `switch`):
  ```cpp
  namespace friendlybot
  {
      class Timer
      {
      public:
          void start()
          {
              if( isStopped() ) {
                  state = State::Running;
              } else {
                  reset();
              }
          }
      };
  }
  ```
- `switch` statements indent `case` labels:
  ```cpp
  switch( mode ) {
      case Mode::Clock:
          renderClock();
          break;
      default:
          break;
  }
  ```

### 2.4 Include Organization
Headers must be grouped cleanly:
1. Local project headers in quotes (`"pins.h"`, `"display.h"`) sorted alphabetically.
2. Standard C/C++ library headers (`<cstdint>`, `<string>`, `<vector>`).
3. External/Framework library headers (`<Arduino.h>`, `<Wire.h>`, `<Adafruit_SSD1306.h>`).

Include guards use `#ifndef` / `#define` standard matching the filename, e.g.:
```cpp
#ifndef FRIENDLYBOT_PINS_H
#define FRIENDLYBOT_PINS_H
...
#endif // FRIENDLYBOT_PINS_H
```

---

## 3. Naming Conventions

| Construct | Case Convention | Example |
| :--- | :--- | :--- |
| **Classes / Structs** | `PascalCase` | `DisplayManager`, `AphorismEngine` |
| **Enums (Scoped)** | `PascalCase` (Enum & Values) | `enum class AppMode { Clock, Timer, Arbitrator };` |
| **Functions / Methods** | `camelCase` | `readLightLevel()`, `renderHeader()` |
| **Local Variables** | `camelCase` | `currentTime`, `distanceMm` |
| **Member Variables** | `camelCase` | `displayDriver`, `lastMovementTime` |
| **Constants / Macros** | `UPPER_SNAKE_CASE` | `PIN_BUZZER`, `SCREEN_WIDTH` |

---

## 4. Modern C++ Best Practices

### 4.1 Constants & Types
- Prefer `static constexpr` or `constexpr` over `#define` for constants.
- Prefer `enum class` over raw `enum`.
- Use explicit fixed-width integers (`uint8_t`, `uint16_t`, `int32_t`) when representing hardware values, byte buffers, and pin numbers.

### 4.2 Attributes & Correctness
- Mark query / getter functions that return values without side effects with `[[nodiscard]]`.
- Apply `const` correctness rigorously on member functions, parameter references, and local constants.
- Use `std::underlying_type_t` or helper `to_underlying()` for enum conversion.

```cpp
[[nodiscard]] bool isUsbPowered() const
{
    return digitalRead( PIN_USB_DETECT ) == HIGH;
}
```
