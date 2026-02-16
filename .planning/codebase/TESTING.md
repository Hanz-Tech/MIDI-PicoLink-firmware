# Testing Patterns

**Analysis Date:** 2026-02-15

## Test Framework

**Runner:** None configured

**No automated testing infrastructure exists in this project.** There are no test files, no test framework, no test configuration, and no CI/CD pipeline.

**Run Commands:**
```bash
# No test commands available
# The project has no test runner or test scripts
```

---

## Current State

### What Exists

**Manual testing only.** The firmware is tested by:
1. Flashing to an RP2040 board via Arduino IDE
2. Observing serial debug output via `Serial2` (UART debug port) and `Serial` (USB CDC)
3. Sending/receiving MIDI messages through physical MIDI devices
4. Using the web configurator (`web_configurator/index.html`) to test configuration read/write

**Debug output infrastructure** (`rp2040/serial_utils.cpp`) provides dual-serial logging that serves as the primary debugging mechanism. The `bool debug = true;` global toggle enables/disables all debug output.

### What Does NOT Exist

- No unit test files (`*.test.*`, `*.spec.*`) — confirmed via glob search
- No test framework dependencies in `web_configurator/package.json` (no Jest, Vitest, Mocha, etc.)
- No test scripts in `package.json`
- No CMakeLists.txt or build system that could support native C++ tests
- No CI/CD pipeline (no `.github/workflows/`, no `.gitlab-ci.yml`)
- No `.eslintrc`, `.prettierrc`, or `.clang-format` for automated code quality checks
- No mocking infrastructure
- No test fixtures or factories
- No coverage tooling

---

## Test File Organization

**Location:** Not applicable — no test files exist.

**Naming:** Not established.

**Structure:** Not established.

---

## Recommendations for Adding Tests

### TypeScript Web App (`web_configurator/`)

The web configurator is the most testable component. To add tests:

**Recommended framework:** Vitest (lightweight, TypeScript-native)

**Install:**
```bash
npm install --save-dev vitest
```

**Add to `web_configurator/package.json` scripts:**
```json
{
  "scripts": {
    "test": "vitest run",
    "test:watch": "vitest"
  }
}
```

**Testable modules:**

1. **`web_configurator/src/validator.ts`** — Pure validation logic with Ajv schemas. Easiest to test:
```typescript
// web_configurator/src/validator.test.ts
import { describe, it, expect } from 'vitest';
import { validateConfig } from './validator';

describe('validateConfig', () => {
  it('accepts valid SAVEALL config', () => {
    const config = {
      command: "SAVEALL",
      filters: [
        [false, false, false, false, false, false, false, false],
        [false, false, false, false, false, false, false, false],
        [false, false, false, false, false, false, false, false],
      ],
      channels: Array(16).fill(true),
    };
    expect(validateConfig(config)).toBe(true);
  });

  it('rejects invalid filter dimensions', () => {
    const config = {
      command: "SAVEALL",
      filters: [[false]],
      channels: Array(16).fill(true),
    };
    expect(validateConfig(config)).toBe(false);
  });
});
```

2. **`web_configurator/src/serial-handler.ts`** — The `SerialHandler` class could be tested with mocked Web Serial API.

3. **`web_configurator/src/app.ts`** — Heavily DOM-dependent; would need JSDOM or similar for testing. Lower priority.

### C++ Firmware (`rp2040/`)

Testing Arduino firmware natively is more complex. Options:

**Option A: Arduino Unit Testing with PlatformIO**
- Migrate from Arduino IDE to PlatformIO for `platformio.ini` build/test support
- Use `unity` or `AUnit` test framework
- Run tests on-device or via emulation

**Option B: Extract Pure Logic for Native Testing**
- Extract testable pure functions into platform-independent modules
- Testable candidates:
  - `rp2040/midi_filters.cpp` — Filter state management (no hardware dependencies)
  - `rp2040/imu_handler.cpp` — `angleToMidiCC()` function (pure math)
  - `rp2040/config.cpp` — `configToJson()` / `updateConfigFromJson()` (requires ArduinoJson mock)
  - `rp2040/usb_host_wrapper.cpp` — `processMidiPacket()` (pure MIDI parsing)

**Pure functions suitable for unit testing:**
```cpp
// rp2040/imu_handler.cpp - angleToMidiCC() is pure
uint8_t angleToMidiCC(float angle, float range, uint8_t defaultValue);

// rp2040/midi_filters.cpp - filter state logic is pure
bool isMidiFiltered(MidiInterfaceType interface, MidiMsgType msgType);
void setMidiFilter(MidiInterfaceType interface, MidiMsgType msgType, bool enabled);

// rp2040/usb_host_wrapper.cpp - MIDI packet parsing is pure
void processMidiPacket(uint8_t packet[4]);
```

---

## Mocking

**Framework:** Not applicable — no tests exist.

**If adding tests for TypeScript:**

**What to Mock:**
- `navigator.serial` (Web Serial API) — for testing `SerialHandler`
- DOM elements — for testing `app.ts` UI logic
- `SerialPort.readable` / `SerialPort.writable` — for testing serial communication

**What NOT to Mock:**
- `validator.ts` — Pure logic, test directly
- Ajv validation schemas — Test against real schema compilation
- JSON serialization/deserialization — Use real JSON

---

## Fixtures and Factories

**Test Data:** Not established.

**If adding tests, create fixtures for:**

```typescript
// web_configurator/src/__fixtures__/configs.ts
export const validConfig = {
  command: "SAVEALL" as const,
  filters: Array(3).fill(Array(8).fill(false)),
  channels: Array(16).fill(true),
};

export const validConfigWithIMU = {
  ...validConfig,
  imu: {
    enabled: true,
    roll: { enabled: true, channel: 1, cc: 1, defaultValue: 64, toSerial: true, toUSBDevice: true, toUSBHost: true, sensitivity: 1.0, range: 45 },
    pitch: { enabled: false, channel: 1, cc: 2, defaultValue: 64, toSerial: true, toUSBDevice: true, toUSBHost: true, sensitivity: 1.0, range: 45 },
    yaw: { enabled: false, channel: 1, cc: 3, defaultValue: 64, toSerial: true, toUSBDevice: true, toUSBHost: true, sensitivity: 1.0, range: 90 },
  },
};
```

**Location:** Place test fixtures alongside test files or in a `__fixtures__/` subdirectory.

---

## Coverage

**Requirements:** None enforced.

**View Coverage:** Not applicable.

---

## Test Types

**Unit Tests:**
- Not implemented
- Primary candidates: `validator.ts`, `angleToMidiCC()`, `isMidiFiltered()`, `processMidiPacket()`

**Integration Tests:**
- Not implemented
- Primary candidate: Web configurator ↔ firmware JSON protocol round-trip testing

**E2E Tests:**
- Not implemented
- Would require physical hardware or emulator for full MIDI routing tests
- Web configurator E2E could use Playwright with mocked Web Serial API

---

## Common Patterns

**Async Testing (recommended for web_configurator):**
```typescript
describe('SerialHandler', () => {
  it('writes data to serial port', async () => {
    const handler = new SerialHandler();
    // ... mock setup ...
    await handler.write('{"command":"READALL"}\n');
    // ... assertions ...
  });
});
```

**Error Testing (recommended for validator):**
```typescript
it('rejects config with wrong channel count', () => {
  const config = {
    command: "SAVEALL",
    filters: Array(3).fill(Array(8).fill(false)),
    channels: Array(10).fill(true), // Wrong: should be 16
  };
  expect(validateConfig(config)).toBe(false);
  expect(validateConfig.errors).toBeDefined();
});
```

---

*Testing analysis: 2026-02-15*
