# Codebase Structure

**Analysis Date:** 2026-02-15

## Directory Layout

```
MIDI-PicoLink-firmware/
├── rp2040/                    # Main firmware (Arduino sketch)
├── web_configurator/          # Browser-based configuration UI
│   ├── src/                   # TypeScript source files
│   ├── dist/                  # Webpack output (gitignored)
│   ├── index.html             # Single-page app entry
│   ├── package.json           # Node.js dependencies
│   ├── tsconfig.json          # TypeScript config
│   └── webpack.config.js      # Build config
├── lsm6ds3/                   # IMU demo/test sketches (LSM6DS3)
│   ├── fastimu.ino            # FastIMU library demo
│   ├── lsm6ds3_angles/        # Angle calculation demo
│   ├── SimpleGyroscope/       # Basic gyroscope demo
│   ├── i2c_scanner/           # I2C address scanner utility
│   └── mpu_6050_angles.ino    # MPU6050 angle demo
├── mpu5060_demo/              # MPU6050 demo/test sketches
│   ├── mpu_6050_angles/       # Angle calculation demo
│   └── MPU6050_README.md      # Documentation
├── .planning/                 # Planning documents
├── arduino-cli.exe            # Arduino CLI binary (Windows)
├── README.md                  # Build instructions
└── .gitignore                 # Git ignore rules
```

## Directory Purposes

**`rp2040/`:**
- Purpose: Main firmware for the MIDI PicoLink device
- Contains: Arduino `.ino` sketch file plus `.cpp`/`.h` module pairs
- Key files: `rp2040.ino` (entry point), `config.cpp` (persistence), `midi_filters.cpp` (filter logic)
- Build: Compiled as an Arduino sketch with arduino-pico board package

**`web_configurator/`:**
- Purpose: Browser-based UI to configure MIDI filters, channels, and IMU settings over Web Serial
- Contains: TypeScript source (`src/`), HTML entry point, build tooling
- Key files: `src/app.ts` (main app logic), `src/serial-handler.ts` (Web Serial abstraction), `src/validator.ts` (JSON schema)
- Build: `npm run build` → webpack bundles `src/app.ts` → `dist/app.js`

**`lsm6ds3/`:**
- Purpose: Standalone demo/test sketches for IMU sensor development
- Contains: Independent Arduino sketches for testing LSM6DS3 and MPU6050 sensors
- Note: Not part of the main firmware — used during development/prototyping

**`mpu5060_demo/`:**
- Purpose: Standalone MPU6050 demo sketches
- Contains: Angle calculation example for MPU6050
- Note: Not part of the main firmware — development reference only

## Key File Locations

**Entry Points:**
- `rp2040/rp2040.ino`: Main firmware sketch — `setup()`, `loop()`, `setup1()`, `loop1()`, and all MIDI message handlers
- `web_configurator/src/app.ts`: Web configurator application entry — DOM event wiring, serial communication orchestration
- `web_configurator/index.html`: Web configurator HTML — loaded directly in browser

**Configuration:**
- `rp2040/config.h`: EEPROM and JSON config function declarations, JSON schema documentation in comments
- `rp2040/config.cpp`: EEPROM save/load, JSON serialization/deserialization for all device settings
- `rp2040/version.h`: Firmware version string (`FIRMWARE_VERSION "1.0.0"`)
- `web_configurator/webpack.config.js`: Webpack bundler config
- `web_configurator/tsconfig.json`: TypeScript compiler config (ES2020, strict mode)
- `web_configurator/package.json`: Node.js dependencies and scripts

**Core MIDI Logic:**
- `rp2040/midi_filters.h`: Filter and channel enums (`MidiInterfaceType`, `MidiMsgType`), function declarations
- `rp2040/midi_filters.cpp`: Filter state management — 2D boolean array + channel array
- `rp2040/midi_instances.h`: USB Device MIDI instance declaration (`USB_D`)
- `rp2040/midi_instances.cpp`: USB Device MIDI instance creation via `MIDI_CREATE_INSTANCE` macro
- `rp2040/usb_host_wrapper.h`: USB Host MIDI function declarations, state variables
- `rp2040/usb_host_wrapper.cpp`: USB Host MIDI packet parsing, TinyUSB callbacks, send helpers
- `rp2040/serial_midi_handler.h`: Serial MIDI function declarations
- `rp2040/serial_midi_handler.cpp`: Serial MIDI instance (`SERIAL_M`), setup, send/receive handlers

**IMU:**
- `rp2040/imu_handler.h`: `IMUConfig` struct definition, function declarations, pin/address constants
- `rp2040/imu_handler.cpp`: IMU initialization, sensor reading, complementary filter, angle-to-CC conversion, JSON config

**Utilities:**
- `rp2040/serial_utils.h`: Dual-serial print function declarations
- `rp2040/serial_utils.cpp`: `dualPrint()`, `dualPrintln()`, `dualPrintf()` implementations
- `rp2040/led_utils.h`: LED pin definitions (`LED_IN_PIN 29`, `LED_OUT_PIN 19`), function declarations
- `rp2040/led_utils.cpp`: LED timing/trigger logic
- `rp2040/web_serial_config.h`: Web Serial config processing declaration
- `rp2040/web_serial_config.cpp`: JSON command processor (READALL, SAVEALL, CALIBRATE_IMU), delayed EEPROM save

**Web Configurator Source:**
- `web_configurator/src/app.ts`: Main application — UI building, connect/send/export/upload logic
- `web_configurator/src/serial-handler.ts`: `SerialHandler` class — Web Serial API wrapper (init, write, readLine, close)
- `web_configurator/src/validator.ts`: AJV JSON Schema validation for `Rp2040Config` type

## Naming Conventions

**Files (firmware):**
- `snake_case.cpp` / `snake_case.h`: All firmware source files use snake_case (e.g., `midi_filters.cpp`, `usb_host_wrapper.h`)
- `snake_case.ino`: Arduino sketch entry point follows same convention
- Exception: `version.h` and `config.h` are single-word names

**Files (web configurator):**
- `kebab-case.ts`: TypeScript source files use kebab-case (e.g., `serial-handler.ts`)
- Exception: `app.ts` and `validator.ts` are single-word names

**Functions (firmware):**
- `camelCase()`: Most functions use camelCase (e.g., `setupMidiFilters()`, `isMidiFiltered()`, `sendSerialMidiNoteOn()`)
- Prefixed by module: `sendSerialMidi*()`, `usbh_on*Handle()`, `usbd_on*()`, `localSerialOn*()`
- Setup/loop pattern: each module has `setup*()` and `loop*()` (e.g., `setupSerialMidi()` / `loopSerialMidi()`, `setupIMU()` / `loopIMU()`)

**Functions (web configurator):**
- `camelCase()`: All TypeScript functions use camelCase (e.g., `buildConfigJson()`, `applyConfigToUI()`, `connectSerial()`)

**Variables (firmware):**
- `camelCase`: Local and global variables (e.g., `midi_dev_addr` uses snake_case for USB host state, `isConnectedToComputer` uses camelCase)
- Constants: `UPPER_SNAKE_CASE` for `#define` macros (e.g., `LED_IN_PIN`, `HOST_PIN_DP`, `CONFIG_EEPROM_SIZE`)

**Types/Enums (firmware):**
- `PascalCase` for typedefs: `MidiInterfaceType`, `MidiMsgType`, `IMUConfig`
- `UPPER_SNAKE_CASE` for enum values: `MIDI_INTERFACE_SERIAL`, `MIDI_MSG_NOTE`

**Types (web configurator):**
- `PascalCase` for TypeScript interfaces: `IMUAxisConfig`, `IMUConfig`, `Rp2040Config`, `SerialHandler`

## Where to Add New Code

**New MIDI message type filter:**
- Add enum value to `MidiMsgType` in `rp2040/midi_filters.h` (before `MIDI_MSG_COUNT`)
- Update `msgTypeNames[]` arrays in `rp2040/midi_filters.cpp`
- Add filter checks in message handlers in `rp2040/rp2040.ino` and `rp2040/serial_midi_handler.cpp`
- Update EEPROM layout in `rp2040/config.cpp` and increase `CONFIG_EEPROM_SIZE` if needed
- Update JSON schema and UI in `web_configurator/src/validator.ts` and `web_configurator/index.html`

**New MIDI interface:**
- Add enum value to `MidiInterfaceType` in `rp2040/midi_filters.h` (before `MIDI_INTERFACE_COUNT`)
- Create new handler file pair: `rp2040/new_interface_handler.h` / `rp2040/new_interface_handler.cpp`
- Follow the pattern from `rp2040/serial_midi_handler.cpp`: provide `setup*()`, `loop*()`, and `send*()` functions
- Add forwarding calls in existing handlers in `rp2040/rp2040.ino`
- Update EEPROM layout and JSON config

**New firmware module:**
- Create `rp2040/module_name.h` and `rp2040/module_name.cpp`
- Follow existing pattern: declare public functions in header with include guards (`#ifndef MODULE_NAME_H`)
- Provide `setup*()` called from `setup()` and `loop*()` called from `loop()` in `rp2040/rp2040.ino`
- Include `serial_utils.h` for debug logging

**New web serial command:**
- Add command handling in `rp2040/web_serial_config.cpp` → `processWebSerialConfig()` function
- Add corresponding button and handler in `web_configurator/src/app.ts`
- Add button element in `web_configurator/index.html`

**New web configurator feature:**
- Add TypeScript logic in `web_configurator/src/app.ts` or create a new `.ts` file in `web_configurator/src/`
- New files are automatically picked up by webpack (imported from `app.ts`)
- Update HTML UI in `web_configurator/index.html`
- If config schema changes, update `web_configurator/src/validator.ts`

## Special Directories

**`rp2040/build/`:**
- Purpose: Arduino build output (compiled binaries, intermediate objects)
- Generated: Yes (by Arduino IDE / arduino-cli)
- Committed: No (gitignored)

**`web_configurator/dist/`:**
- Purpose: Webpack-bundled JavaScript output
- Generated: Yes (by `npm run build`)
- Committed: No (gitignored via top-level `dist` rule)

**`web_configurator/node_modules/`:**
- Purpose: Node.js dependencies
- Generated: Yes (by `npm install`)
- Committed: No (gitignored)

**`lsm6ds3/` and `mpu5060_demo/`:**
- Purpose: Standalone development/prototyping sketches for IMU sensors
- Generated: No (hand-written reference code)
- Committed: Yes
- Note: These are independent Arduino sketches, not compiled as part of the main firmware

**`.planning/`:**
- Purpose: Project planning and analysis documents
- Generated: By planning tools
- Committed: Yes

---

*Structure analysis: 2026-02-15*
