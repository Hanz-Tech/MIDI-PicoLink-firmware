# Coding Conventions

**Analysis Date:** 2026-02-15

## Languages & Dual Codebase

This project has **two distinct codebases** with different conventions:

1. **Arduino/C++ firmware** (`rp2040/`) - RP2040 microcontroller firmware
2. **TypeScript web app** (`web_configurator/src/`) - Browser-based configuration tool

Apply the correct conventions based on which codebase you are modifying.

---

## Naming Patterns

### C++ Firmware (`rp2040/`)

**Files:**
- Use `snake_case` for all file names: `midi_filters.cpp`, `serial_utils.h`, `led_utils.cpp`
- Each module has a paired `.h` and `.cpp` file
- Exception: the main Arduino sketch is `rp2040.ino`

**Functions:**
- Use `camelCase` for all function names: `setupMidiFilters()`, `sendNoteOn()`, `handleLEDs()`
- Prefix with module context for clarity: `sendSerialMidiNoteOn()`, `usbh_onNoteOnHandle()`
- Use `setup*` for initialization functions: `setupIMU()`, `setupSerialMidi()`, `setupMidiFilters()`
- Use `loop*` for functions called in the main loop: `loopIMU()`, `loopSerialMidi()`
- Use `handle*` for functions that process state: `handleLEDs()`, `handleDelayedEEPROMSave()`
- Prefix USB Host handler callbacks with `usbh_on*Handle`: `usbh_onNoteOnHandle()`
- Prefix USB Device handler callbacks with `usbd_on*`: `usbd_onNoteOn()`
- Prefix Serial handler callbacks with `localSerialOn*`: `localSerialOnNoteOn()`

**Variables:**
- Use `camelCase` for local and global variables: `isConnectedToComputer`, `midi_dev_addr`
- Use `SCREAMING_SNAKE_CASE` for `#define` constants: `LED_IN_PIN`, `MIDI_MSG_COUNT`, `HOST_PIN_DP`
- Use `SCREAMING_SNAKE_CASE` for enum values: `MIDI_MSG_NOTE`, `MIDI_INTERFACE_SERIAL`
- Prefix static module-level state with descriptors: `static bool inLedActive`, `static float gyroXoffset`

**Types/Enums:**
- Use `typedef enum` with `PascalCase` type names: `MidiMsgType`, `MidiInterfaceType`
- Use `typedef struct` with `PascalCase` type names: `IMUConfig`
- Enum members use `SCREAMING_SNAKE_CASE`: `MIDI_MSG_NOTE`, `MIDI_INTERFACE_USB_HOST`
- Include a `*_COUNT` sentinel at the end of enums: `MIDI_MSG_COUNT`, `MIDI_INTERFACE_COUNT`

**Global Instance Names:**
- Use `SCREAMING_SNAKE_CASE` for MIDI instances: `USB_D`, `SERIAL_M`
- Use `PascalCase` for hardware singletons: `USBHost`, `IMU`

### TypeScript Web App (`web_configurator/src/`)

**Files:**
- Use `kebab-case` for TypeScript files: `serial-handler.ts`, `validator.ts`
- Main entry point is `app.ts`

**Functions:**
- Use `camelCase` for all functions: `buildConfigJson()`, `applyConfigToUI()`, `connectSerial()`
- Prefix logging helpers with `log*`: `logSent()`, `logRecv()`, `logInfo()`, `logError()`

**Variables:**
- Use `camelCase` for variables: `serialHandler`, `statusDiv`, `sendBtn`

**Types/Interfaces:**
- Use `PascalCase` for interfaces: `IMUAxisConfig`, `IMUConfig`, `Rp2040Config`
- Use TypeScript `interface` (not `type` alias) for data shapes

**Exports:**
- Use named exports: `export class SerialHandler`, `export const serialHandler`, `export const validateConfig`
- Export both class and singleton instance: `export class SerialHandler` + `export const serialHandler = new SerialHandler()`

---

## Code Style

### C++ Firmware

**Formatting:**
- No automated formatter configured (no `.clang-format` file)
- Use 2-space or 4-space indentation (mixed in codebase; prefer 4-space for new code)
- Opening braces on the same line as function/control statement
- One blank line between function definitions

**Include Guard Pattern:**
- Always use `#ifndef` / `#define` / `#endif` include guards
- Name format: `FILENAME_H` (uppercase filename with underscore): `CONFIG_H`, `MIDI_FILTERS_H`, `LED_UTILS_H`

```cpp
#ifndef MIDI_FILTERS_H
#define MIDI_FILTERS_H

// ... declarations ...

#endif // MIDI_FILTERS_H
```

### TypeScript Web App

**Formatting:**
- No ESLint or Prettier configured
- Use 2-space indentation (consistent throughout)
- TypeScript strict mode is enabled: `"strict": true` in `web_configurator/tsconfig.json`

**Linting:**
- No linting tool configured
- TypeScript compiler strict mode provides basic enforcement

---

## Import Organization

### C++ Firmware (`rp2040/*.cpp`)

**Order:**
1. Standard library headers: `#include <cstdint>`, `#include <stdarg.h>`, `#include <math.h>`
2. Arduino/platform headers: `#include <Arduino.h>`, `#include <Wire.h>`, `#include <EEPROM.h>`
3. Third-party library headers: `#include <MIDI.h>`, `#include <Adafruit_TinyUSB.h>`, `#include <ArduinoJson.h>`, `#include "FastIMU.h"`
4. Project-local headers: `#include "config.h"`, `#include "midi_filters.h"`, `#include "serial_utils.h"`

Use angle brackets `<>` for platform/library headers and quotes `""` for project headers.

### TypeScript (`web_configurator/src/*.ts`)

**Order:**
1. Third-party imports: `import Ajv from "ajv"`
2. Local imports: `import { serialHandler } from "./serial-handler"`

**Path Aliases:**
- No path aliases configured; use relative imports: `"./serial-handler"`, `"./validator"`

---

## Error Handling

### C++ Firmware

**Patterns:**
- Functions that can fail return `bool`: `sendMidiPacket()` returns `false` if not mounted
- Boundary checks with early return: validate array indices before access

```cpp
bool isMidiFiltered(MidiInterfaceType interface, MidiMsgType msgType) {
    if (interface < MIDI_INTERFACE_COUNT && msgType < MIDI_MSG_COUNT) {
        return midiFilters[interface][msgType];
    }
    return false; // Default to not filtered if invalid parameters
}
```

- MIDI message handlers use early return for filtering:

```cpp
void usbh_onNoteOnHandle(byte channel, byte note, byte velocity) {
    if (!isChannelEnabled(channel)) return;          // Channel filter
    if (isMidiFiltered(MIDI_INTERFACE_USB_HOST, MIDI_MSG_NOTE)) return; // Type filter
    // ... process message
}
```

- JSON parsing validates structure before use:

```cpp
if (filtersArr.isNull()) {
    Serial.println("[DEBUG] updateConfigFromJson: 'filters' is not an array or missing");
    return false;
}
```

- Use `#error` preprocessor directives for build-time checks:

```cpp
#if defined(USE_TINYUSB_HOST) || !defined(USE_TINYUSB)
#error "Please use the Menu to select Tools->USB Stack: Adafruit TinyUSB"
#endif
```

### TypeScript Web App

**Patterns:**
- Use `try/catch` blocks for async serial operations
- Catch errors as `any` type: `catch (err: any)`
- Display errors in both the status div and the log box
- Validate JSON configuration with Ajv schema validation before sending

```typescript
try {
    const config = buildConfigJson();
    if (!validateConfig(config)) {
        logError("Invalid config: " + JSON.stringify(validateConfig.errors));
        return;
    }
    // ... proceed
} catch (err: any) {
    statusDiv.textContent = "Error: " + err;
    logError("" + err);
}
```

---

## Logging

### C++ Firmware

**Framework:** Custom dual-output logging via `rp2040/serial_utils.h` and `rp2040/serial_utils.cpp`

**Pattern:** All debug output goes to BOTH `Serial` (USB CDC) and `Serial2` (hardware UART debug port) simultaneously:
- `dualPrint(message)` - Print without newline
- `dualPrintln(message)` - Print with newline
- `dualPrintf(format, ...)` - Printf-style formatted output (256-byte buffer)

**When to log:**
- Log state changes: filter changes, connection/disconnection events
- Log MIDI messages with channel/note/velocity details for debugging
- Prefix debug messages with `[DEBUG]`, `[IMU DEBUG]`, `[IMU MIDI]` tags
- Suppress high-frequency messages (e.g., MIDI Clock) to avoid flooding

```cpp
dualPrintf("USB Host: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
dualPrintf("[DEBUG] Saving IMU config: Roll en=%d, ch=%d, cc=%d\n", 
           imuConfig.rollEnabled, imuConfig.rollMidiChannel, imuConfig.rollMidiCC);
```

**Global debug toggle:** `bool debug = true;` in `rp2040/serial_utils.cpp` controls whether logging is active.

### TypeScript Web App

**Framework:** Custom log functions that append to a `<pre>` element in the DOM

**Pattern:** Categorized log messages with prefix tags:
- `logSent(msg)` → `[SENT] ...`
- `logRecv(msg)` → `[RECV] ...`
- `logInfo(msg)` → `[INFO] ...`
- `logError(msg)` → `[ERROR] ...`

**Console:** Use `console.error()` for fatal errors (e.g., serial port open failure).

---

## Comments

### C++ Firmware

**When to Comment:**
- Add section headers for logical groupings: `// --- USB Host MIDI message handlers ---`
- Comment on hardware pin assignments: `#define HOST_PIN_DP 12 // Pin used as D+ for host`
- Use inline comments for MIDI protocol constants: `case 0x80: // Note Off`
- Add `// MOVED TO ...` or `// REMOVED` comments when refactoring code between modules
- Document JSON protocol format in header file comments (see `rp2040/config.h` lines 17-53)

**License headers:**
- MIT license block at top of `rp2040/usb_host_wrapper.h` (Doxygen-style `@brief`, `@author`, `@date`)
- Not consistently applied across all files

### TypeScript Web App

**When to Comment:**
- JSDoc-style header comment on `SerialHandler` class with usage example
- Inline comments for non-obvious logic: `// REVERSED LOGIC: checked = allowed = true`

---

## Function Design

### C++ Firmware

**Size:** Functions are generally small (5-30 lines). MIDI message handler functions follow a repeated pattern.

**Parameters:** Use Arduino-standard types: `byte`, `uint8_t`, `int`, `unsigned`, `float`. Pass structs by const reference: `const IMUConfig &config`.

**Return Values:**
- `bool` for success/failure: `sendNoteOn()`, `setupIMU()`
- `void` for handlers and setup functions
- Return by value for small structs: `IMUConfig getIMUConfig()`

### TypeScript Web App

**Size:** Functions range from small helpers (3 lines) to medium (30-50 lines). 

**Parameters:** Use typed parameters. DOM elements are cast with `as HTMLInputElement`.

**Return Values:**
- `Promise<boolean>` for async init
- `Promise<void>` for async operations
- Plain object returns for `buildConfigJson()`

---

## Module Design

### C++ Firmware

**Exports (header files):**
- Every module exposes its public API through its `.h` file
- Use `extern` for global variable declarations in headers
- Keep implementation details `static` in `.cpp` files

**Pattern:**
- Each module has `setup*()` and `loop*()` or `handle*()` functions
- Modules communicate through function calls, not shared globals (except `isConnectedToComputer`, `midi_dev_addr`, `midi_host_mounted`)
- Use `extern` declarations to access cross-module state

### TypeScript Web App

**Exports:**
- Named exports for classes and singleton instances
- Validator module exports compiled Ajv validation function directly
- No barrel files

**Barrel Files:** Not used.

---

## JSON Communication Protocol

The firmware and web configurator share a JSON-based protocol over USB Serial:

**Commands from web → firmware:**
- `{"command": "READALL"}` — Request current config
- `{"command": "SAVEALL", "filters": [...], "channels": [...], "imu": {...}}` — Save config
- `{"command": "CALIBRATE_IMU"}` — Start IMU calibration

**Responses from firmware → web:**
- `{"status": "Success", "command": "SAVEALL", ...}` — Success
- `{"status": "Invalid config JSON", ...}` — Error
- Full config JSON with `"version"` field on READALL

---

*Convention analysis: 2026-02-15*
