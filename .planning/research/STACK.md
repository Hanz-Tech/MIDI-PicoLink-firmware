# Stack Research

**Domain:** RP2040 MIDI Router — CV/Clock Output, EEPROM Versioning, Code Refactoring
**Researched:** 2026-02-15
**Confidence:** HIGH (existing stack verified against codebase; new additions verified against official docs)

## Recommended Stack

### Core Technologies (Existing — Keep As-Is)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Arduino-Pico | 5.5.0 | RP2040 board support package | Latest stable (Jan 2026). Already in use. Provides EEPROM emulation (4096 bytes), PWM via analogWrite/pico-sdk, dual-core support. No reason to upgrade — 5.5.0 is current. |
| Adafruit TinyUSB | 3.7.4 | USB Host + Device MIDI stack | Only viable option for simultaneous USB Host + Device on RP2040 with Arduino. Already integrated. |
| MIDI Library | 5.0.2 | MIDI message parsing and routing | Standard Arduino MIDI library. Handles Serial and USB MIDI instances. Already working. |
| ArduinoJson | 7.4.1 | JSON config protocol serialization | Already in use for Web Serial config. Sufficient for adding CV/clock config fields. |
| Pico PIO USB | 0.7.2 | USB Host via PIO bit-banging | Required for USB Host on GPIO 12/13. Hardware-specific, no alternatives. |

### New Technologies for CV/Clock Output

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Pico SDK `hardware/pwm.h` | (bundled with Arduino-Pico 5.5.0) | Direct PWM hardware control for CV output | **Use this instead of `analogWrite()`.** `analogWrite()` has a global frequency setting and limited control. The pico-sdk PWM API provides per-slice configuration: independent wrap value, clock divider, and duty cycle per channel. At 120MHz sysclk, you can configure a PWM slice for ~29.3kHz with 12-bit (4096-step) resolution, which after the Sallen-Key LP filter (fc=9947Hz, Q=0.5) produces clean CV. The Arduino-pico core explicitly supports mixing pico-sdk calls with Arduino code. |
| Pico SDK `hardware/gpio.h` | (bundled) | Clock output GPIO control | For Eurorack clock output (ring of TRS). Simple digital GPIO toggle with `gpio_put()` — no PWM needed. Clock is a 5V/0V square wave produced by the op-amp gain stage. Faster and more deterministic than `digitalWrite()`. |
| No new libraries needed | — | — | CV output is pure hardware PWM. Clock is GPIO toggle. Both use pico-sdk APIs already bundled in Arduino-Pico. Zero new dependencies. |

### Supporting Libraries (No Changes)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| EEPROM | 1.0 (bundled) | Persistent config storage | Already in use. Supports up to 4096 bytes (emulated in flash). Current 100-byte allocation will grow to ~200-300 bytes with CV/clock config + versioning header. |
| FastIMU | (bundled) | IMU sensor abstraction | Already integrated. Keep unless it breaks on a future Arduino-Pico update. |
| Wire | (built-in) | I2C for IMU | Already in use (Wire1, pins 26/27). |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Arduino CLI | Firmware compilation | Already in use. Compile with `--fqbn rp2040:rp2040:rpipico:usbstack=tinyusb`. Ensure 120MHz CPU speed. |
| Serial2 (GPIO 24/25) | Hardware debug UART | Already wired. Will become the sole debug output channel after refactoring debug logging away from USB CDC Serial. |

## PWM CV Output: Technical Details

### Why Pico SDK PWM API Instead of analogWrite()

**Confidence: HIGH** (verified against Arduino-Pico 5.5.0 official docs + RP2040 datasheet)

The Arduino-Pico `analogWrite()` function has critical limitations for CV output:

1. **Global frequency**: `analogWriteFreq()` sets frequency for ALL PWM pins. If any other feature uses PWM (e.g., LED dimming, Servo), they share the same frequency. CV needs a specific frequency/resolution trade-off.
2. **Limited resolution control**: `analogWriteRange()` sets a global range. At the CV-optimal frequency (~29kHz), you lose bits of resolution with the global setting.
3. **No per-channel control**: Both channels A and B on a PWM slice share the same wrap/frequency but can have independent duty cycles. `analogWrite()` doesn't expose this.

The pico-sdk `hardware/pwm.h` API provides:
- `pwm_set_wrap(slice, wrap)` — sets the counter wrap value (resolution) per slice
- `pwm_set_chan_level(slice, chan, level)` — sets duty cycle per channel
- `pwm_set_clkdiv(slice, div)` — sets clock divider per slice
- `pwm_set_enabled(slice, true)` — enables the PWM slice

**Calculation for 1V/oct CV at 120MHz sysclk:**
```
PWM frequency = sysclk / (clkdiv * (wrap + 1))
Target: maximize resolution while keeping f_pwm > 2 * f_filter_cutoff (9947Hz)
  
Option A (recommended): wrap = 4095 (12-bit), clkdiv = 1.0
  f_pwm = 120,000,000 / (1.0 * 4096) = 29,296.875 Hz
  Resolution: 12 bits = 4096 steps
  After 1.51x gain: 3.3V * 1.51 = 4.983V full scale
  Step size: 4.983V / 4096 = 1.217 mV per step
  1V/oct semitone = 83.33 mV → 83.33 / 1.217 = 68.5 steps per semitone
  Accuracy: ~1.46% per semitone (acceptable for most Eurorack use)
  
Option B (higher resolution): wrap = 8191 (13-bit), clkdiv = 1.0
  f_pwm = 120,000,000 / (1.0 * 8192) = 14,648.4 Hz  
  Resolution: 13 bits = 8192 steps
  Step size: 4.983V / 8192 = 0.608 mV per step
  Accuracy: ~0.73% per semitone (better, but f_pwm closer to filter cutoff)
  
Recommendation: Start with Option A (12-bit). The Sallen-Key filter at 9947Hz 
with Q=0.5 attenuates 29.3kHz by ~20dB — sufficient for clean CV. If tuning 
accuracy is insufficient in testing, Option B is viable since 14.6kHz is still 
above the filter cutoff, just with less attenuation margin.
```

### Pin Allocation for CV/Clock

**Confidence: MEDIUM** (pin choice depends on which GPIO/PWM slices are free; verified existing pin map)

Existing pin allocations:
- GPIO 0/1: Serial MIDI TX/RX
- GPIO 12/13: USB Host PIO D+/D-
- GPIO 19: LED OUT
- GPIO 24/25: Debug UART Serial2 TX/RX
- GPIO 26/27: I2C SDA/SCL (IMU)
- GPIO 29: LED IN

Available PWM-capable pins (any GPIO can do PWM on RP2040):
- GPIO 2-11, 14-18, 20-23, 28 are potentially free

**Constraint:** CV (tip) and Clock (ring) share a TRS jack. They need two separate GPIO pins. CV requires a PWM-capable pin (all GPIO are). Clock only needs digital output.

**Important PWM slice constraint:** Each RP2040 PWM slice has two channels (A and B). Pins sharing a slice share the same frequency/wrap. If CV uses GPIO X, the other pin on the same slice (X+1 or X-1) must not be used for a different PWM frequency.

Pin recommendation will depend on PCB routing — this is a hardware decision, not a software one. Any free GPIO works.

## Eurorack Clock Output: Technical Details

### How MIDI Clock Maps to Eurorack Clock

**Confidence: HIGH** (well-established standard in MIDI/Eurorack domain)

MIDI Clock sends 24 PPQN (pulses per quarter note). Eurorack clock is typically:
- **1 PPQN** (one pulse per quarter note) — most common
- **2 PPQN** (Korg standard)
- **4 PPQN** (sixteenth notes)
- **24 PPQN** (raw MIDI clock passthrough)

Implementation approach:
```
MIDI Clock tick counter → divide by N → set GPIO HIGH for pulse_width_ms → set GPIO LOW
```

Where `N` is configurable (default: 24 for 1 PPQN quarter notes).

**Pulse width:** Typically 10-20ms HIGH pulse, or 50% duty cycle of the divided clock. Both approaches are standard in Eurorack. A configurable pulse width (5-50ms) is ideal.

**Non-blocking implementation:** Use `millis()` or `micros()` to track pulse timing. Never use `delay()` in the clock path — it would block MIDI processing.

**Voltage:** RP2040 GPIO output is 3.3V. Through the OPA2335 non-inverting gain stage (1.51x), this becomes ~5V — standard Eurorack clock level.

### Clock Source Options

The clock output should respond to:
1. **Incoming MIDI Clock** from any interface (USB Host, USB Device, Serial) — most common use case
2. **Internal clock generator** (optional/future) — user sets BPM, device generates clock independently
3. **Transport messages** (Start/Stop/Continue) should control clock gate (stop = hold LOW)

## EEPROM Versioning Strategy

### Current State

**Confidence: HIGH** (verified by reading `config.cpp`)

Current EEPROM layout has NO versioning:
- 100 bytes allocated (`CONFIG_EEPROM_SIZE`)
- Sequential byte writes starting at address 0
- No magic bytes, no version field, no checksum
- ~67 bytes used (24 filters + 16 channels + 27 IMU)

### Recommended EEPROM Header Format

```
Byte 0-1: Magic bytes (0x4D, 0x50 = "MP" for MIDI PicoLink)
Byte 2:   Schema version (uint8_t, start at 1)
Byte 3:   Data length (uint8_t, bytes of payload after header)
Byte 4:   CRC8 checksum of bytes 5..N (simple, fast, sufficient)
Byte 5+:  Payload data (current layout follows here)
```

**Total header overhead: 5 bytes.** Minimal.

### CRC8 Implementation

**Confidence: HIGH** (standard algorithm, no library needed)

Use CRC-8/MAXIM (polynomial 0x31) — well-known, simple to implement in ~10 lines of C:

```cpp
uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}
```

No external library needed. ~200 bytes of flash.

### Migration Strategy

On `loadConfigFromEEPROM()`:
1. Read bytes 0-1. If not magic bytes → first boot or pre-versioned firmware → load defaults, save with header.
2. Read version byte. If current version → load normally.
3. If older version → run migration function for that version → save with current version.
4. Validate CRC. If mismatch → corrupted data → load defaults, save.

**New `CONFIG_EEPROM_SIZE`:** Increase to 512 bytes. Provides room for CV/clock config + future expansion. Still well within the 4096-byte limit.

### What to Add for CV/Clock Config

```
CV config (~10 bytes):
  - CV enabled (1 byte)
  - CV source: note-pitch or CC (1 byte)  
  - CV MIDI channel (1 byte)
  - CV CC number if CC mode (1 byte)
  - CV transpose/offset (1 byte, signed)
  - CV pin (1 byte)
  - Reserved (4 bytes)

Clock config (~8 bytes):
  - Clock enabled (1 byte)
  - Clock division (1 byte: 1/2/4/6/8/12/24 PPQN)
  - Clock pulse width ms (1 byte)
  - Clock pin (1 byte)
  - Clock source interface bitmask (1 byte)
  - Reserved (3 bytes)
```

## Compile-Time Debug Logging

### Current Problem

**Confidence: HIGH** (verified by reading `serial_utils.cpp`)

Debug logging is controlled by `bool debug = true` — a runtime flag. This means:
1. All `dualPrintf()` calls still evaluate their arguments even when debug is off
2. The 256-byte stack buffer is still allocated per call
3. `vsnprintf` formatting still runs (it just doesn't print)
4. Every MIDI handler still calls debug functions, adding function call overhead
5. Debug output goes to `Serial` (USB CDC) which conflicts with Web Serial config protocol

### Recommended Approach: Compile-Time Macros

```cpp
// In a new debug.h header:

// Define DEBUG_LEVEL: 0=off, 1=error, 2=warn, 3=info, 4=debug
#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL 0  // Default: OFF in production builds
#endif

#if DEBUG_LEVEL >= 4
  #define DBG(fmt, ...) Serial2.printf(fmt, ##__VA_ARGS__)
#else
  #define DBG(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 3
  #define INFO(fmt, ...) Serial2.printf(fmt, ##__VA_ARGS__)
#else
  #define INFO(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 2
  #define WARN(fmt, ...) Serial2.printf(fmt, ##__VA_ARGS__)
#else
  #define WARN(fmt, ...) ((void)0)
#endif

#if DEBUG_LEVEL >= 1
  #define ERR(fmt, ...) Serial2.printf(fmt, ##__VA_ARGS__)
#else
  #define ERR(fmt, ...) ((void)0)
#endif
```

**Key decisions:**
1. **Output to Serial2 ONLY** (hardware UART on GPIO 24/25) — never to USB CDC `Serial`. This eliminates the conflict with Web Serial config protocol entirely.
2. **Compile-time elimination** via preprocessor macros — zero overhead when disabled. No stack buffers, no function calls, no argument evaluation.
3. **Leveled logging** — keep ERROR/WARN even in production for critical diagnostics via Serial2.
4. **Enable via build flag:** `arduino-cli compile ... --build-property build.extra_flags=-DDEBUG_LEVEL=4`

**What NOT to do:**
- Don't use a logging library (e.g., ArduinoLog). It adds ~2KB flash overhead, uses virtual functions, and provides features (timestamps, formatting) not needed on an embedded system with no RTC.
- Don't keep runtime `debug` flag — it wastes cycles on every MIDI message even when off.
- Don't log to USB CDC `Serial` — it shares the channel with Web Serial config.

## Embedded C++ Refactoring Patterns

### Generic MIDI Router Pattern

**Confidence: HIGH** (well-established pattern; matches PROJECT.md requirements)

Replace 33+ duplicated handlers with a single routing function:

```cpp
// midi_router.h
enum class MidiPort : uint8_t {
    USB_HOST = 0,
    USB_DEVICE,
    SERIAL,
    CV_CLOCK,     // New output-only interface
    PORT_COUNT
};

struct MidiMessage {
    MidiPort source;
    MidiMsgType type;
    uint8_t channel;    // 1-16, 0 for system messages
    uint8_t data1;      // note/CC number/program
    uint8_t data2;      // velocity/value
    const uint8_t* sysex;  // for SysEx only
    uint16_t sysexLen;
};

// Single entry point for all MIDI routing
void routeMidiMessage(const MidiMessage& msg);
```

The `routeMidiMessage()` function:
1. Checks channel filter (if applicable)
2. Checks source interface filter
3. For each destination != source: checks destination filter → sends
4. Triggers LED
5. Optionally logs (via DBG macro)

Each handler callback becomes a single-line call:
```cpp
void usbh_onNoteOn(byte ch, byte note, byte vel) {
    routeMidiMessage({MidiPort::USB_HOST, MIDI_MSG_NOTE, ch, note, vel});
}
```

### Send Function Dispatch Table

Instead of `if/else` chains for each destination, use a function pointer table:

```cpp
using SendFn = void(*)(const MidiMessage&);
SendFn senders[PORT_COUNT] = {
    sendToUsbHost,
    sendToUsbDevice,
    sendToSerial,
    sendToCvClock,
};
```

This makes adding CV/Clock output a matter of implementing one `sendToCvClock()` function and adding it to the table.

### Non-Blocking Patterns

Replace all `delay()` calls with state machines:

```cpp
// Non-blocking LED blink pattern
struct BlinkState {
    uint32_t nextToggle = 0;
    uint8_t remainingBlinks = 0;
    bool ledOn = false;
};

void updateBlink(BlinkState& state, uint8_t pin) {
    if (state.remainingBlinks == 0) return;
    if (millis() < state.nextToggle) return;
    state.ledOn = !state.ledOn;
    digitalWrite(pin, state.ledOn ? HIGH : LOW);
    state.nextToggle = millis() + BLINK_INTERVAL_MS;
    if (!state.ledOn) state.remainingBlinks--;
}
```

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `analogWrite()` for CV | Global frequency, no per-slice control, resolution tied to global setting | Pico SDK `hardware/pwm.h` for direct PWM slice control |
| External DAC (MCP4725, etc.) | Adds I2C bus contention with IMU, ~1ms latency per I2C transaction, additional BOM cost | PWM + existing analog filter is sufficient for 1V/oct; hardware already designed |
| ArduinoLog library | 2KB+ flash overhead, runtime dispatch, virtual functions — overkill for embedded debug | Simple `#define` macros with compile-time elimination |
| `delay()` in any loop code | Blocks MIDI processing, causes clock jitter, prevents concurrent LED/config operations | `millis()`-based state machines for all timing |
| FreeRTOS | Massive overhead for this use case, conflicts with Arduino-pico's dual-core model, adds stack/RAM pressure | Arduino-pico's native `setup1()/loop1()` dual-core + cooperative state machines |
| `Serial` (USB CDC) for debug | Conflicts with Web Serial config protocol, corrupts JSON responses | `Serial2` (hardware UART on GPIO 24/25) exclusively for debug output |
| `EEPROM.put()/get()` for versioned storage | Writes entire struct without validation or migration capability | Manual byte-level read/write with explicit header, version, and CRC |
| `String` class for debug formatting | Heap fragmentation on embedded systems, non-deterministic allocation | `printf`-style formatting into stack buffers, or compile-time-eliminated macros |
| Timer interrupts for clock output | Complex interaction with PIO USB interrupts, risks USB stack instability | `millis()`/`micros()` polling in main loop — clock jitter of <1ms is acceptable for Eurorack |

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Pico SDK PWM API | Arduino `analogWrite()` | Only if you need single PWM output with no other PWM users and don't care about resolution |
| 12-bit PWM resolution (wrap=4095) | 13-bit (wrap=8191) or 16-bit (wrap=65535) | If 12-bit CV accuracy proves insufficient in testing — trade PWM frequency for resolution |
| CRC-8 for EEPROM integrity | CRC-16 or CRC-32 | Only if EEPROM data exceeds ~256 bytes of payload. CRC-8 has 1/256 collision probability, fine for <200 bytes |
| Compile-time debug macros | Runtime log level flag | If you need to toggle logging without recompiling — but this adds overhead to every log call |
| `millis()` polling for clock | Hardware timer interrupt | If sub-millisecond clock accuracy becomes critical — test `millis()` approach first |
| EEPROM (flash emulation) | LittleFS for config storage | If config data grows beyond 1KB or needs multiple config files — extreme overkill for current needs |

## Stack Patterns by Variant

**If adding only CV output (no clock):**
- Use a single PWM slice channel
- CV config stored in EEPROM after existing layout (with header)
- Responds to MIDI Note On/Off from routed messages
- No timing-critical code needed beyond PWM duty cycle updates

**If adding CV + Clock output (planned):**
- CV on PWM channel, clock on separate GPIO
- Clock divider counter triggered by MIDI Clock messages flowing through the router
- Both configs stored in EEPROM with version header
- Clock pulse timing via `micros()` for tighter timing than `millis()`

**If refactoring MIDI router first (recommended order):**
- Implement `routeMidiMessage()` with existing 3 interfaces
- Verify all existing functionality preserved
- Then add CV/Clock as a 4th output interface — trivial to add with router pattern
- This order prevents duplicating the handler mess for CV/Clock

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| Arduino-Pico 5.5.0 | Adafruit TinyUSB 3.7.4 | TinyUSB is bundled/updated with Arduino-Pico releases. 5.5.0 ships with TinyUSB 3.7.2 in the core; installing TinyUSB 3.7.4 as a separate library overrides it. Both work. |
| Arduino-Pico 5.5.0 | MIDI Library 5.0.2 | Fully compatible. MIDI Library is transport-agnostic. |
| Arduino-Pico 5.5.0 | ArduinoJson 7.4.1 | Fully compatible. ArduinoJson 7.x uses `JsonDocument` without template size parameter. |
| Arduino-Pico 5.5.0 | Pico SDK PWM API | Bundled. The SDK is included in Arduino-Pico. Include `hardware/pwm.h` directly. No version conflict possible. |
| Pico SDK PWM | Arduino `analogWrite()` | **CONFLICT**: Using pico-sdk PWM functions on a pin disables Arduino `analogWrite()` control for that pin's PWM slice. Don't mix both for the same slice. Fine to use pico-sdk for CV and Arduino for other pins on different slices. |

## Installation

No new libraries to install. All capabilities come from:

```bash
# Already installed — no changes needed:
# - Arduino-Pico 5.5.0 (Board Manager)
# - Adafruit TinyUSB Library 3.7.4 (Library Manager)
# - MIDI Library 5.0.2 (Library Manager)
# - ArduinoJson 7.4.1 (Library Manager)
# - Pico PIO USB 0.7.2 (Library Manager)

# To enable debug logging during development:
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb \
  --build-property "build.extra_flags=-DDEBUG_LEVEL=4" \
  ./rp2040

# Production build (debug off, default):
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb \
  ./rp2040
```

## New Headers to Create

| File | Purpose |
|------|---------|
| `rp2040/debug.h` | Compile-time debug logging macros (DBG, INFO, WARN, ERR) |
| `rp2040/midi_router.h` / `.cpp` | Generic MIDI message routing function + dispatch table |
| `rp2040/cv_output.h` / `.cpp` | PWM CV output (1V/oct) and clock output control |
| `rp2040/eeprom_config.h` / `.cpp` | Versioned EEPROM with magic bytes, CRC, migration |

## Sources

- Arduino-Pico 5.5.0 official docs — EEPROM API: https://arduino-pico.readthedocs.io/en/latest/eeprom.html (HIGH confidence)
- Arduino-Pico 5.5.0 official docs — Analog/PWM: https://arduino-pico.readthedocs.io/en/latest/analog.html (HIGH confidence)
- Arduino-Pico 5.5.0 official docs — Using Pico SDK: https://arduino-pico.readthedocs.io/en/latest/sdk.html (HIGH confidence)
- Arduino-Pico 5.5.0 official docs — Multicore: https://arduino-pico.readthedocs.io/en/latest/multicore.html (HIGH confidence)
- Arduino-Pico 5.5.0 release notes: https://github.com/earlephilhower/arduino-pico/releases/tag/5.5.0 (HIGH confidence)
- RP2040 datasheet — PWM chapter: PWM has 8 slices, 16 channels, 16-bit counter, fractional clock divider. At 120MHz, wrap=4095 gives 29.3kHz. (HIGH confidence)
- Existing codebase analysis: `.planning/codebase/STACK.md`, `.planning/codebase/CONCERNS.md`, `.planning/codebase/ARCHITECTURE.md` (HIGH confidence)
- MIDI Clock standard: 24 PPQN is defined in MIDI 1.0 specification (HIGH confidence)
- Eurorack clock conventions: 1 PPQN, 5V pulse, ~10-20ms pulse width (MEDIUM confidence — community standard, not formally specified)
- 1V/oct standard: 1 volt per octave, 83.33mV per semitone (HIGH confidence — Eurorack standard)

---
*Stack research for: MIDI PicoLink CV/Clock Milestone*
*Researched: 2026-02-15*
