# Architecture

**Analysis Date:** 2026-02-15

## Pattern Overview

**Overall:** Dual-core event-driven MIDI router with message-handler forwarding pattern

**Key Characteristics:**
- RP2040 dual-core architecture: Core 0 handles USB Device MIDI, Serial MIDI, Web Serial config, IMU, and LEDs; Core 1 handles USB Host MIDI exclusively
- Callback-based MIDI message routing: each MIDI interface registers handlers that forward messages to the other two interfaces
- Per-interface, per-message-type filtering layer sits between message reception and forwarding
- JSON-over-USB-Serial configuration protocol allows a web browser to read/write device settings
- EEPROM persistence with delayed save to avoid USB Host interference

## Layers

**MIDI Transport Layer:**
- Purpose: Handles raw MIDI communication across three interfaces (USB Host, USB Device, Serial)
- Location: `rp2040/usb_host_wrapper.cpp`, `rp2040/serial_midi_handler.cpp`, `rp2040/midi_instances.cpp`
- Contains: USB packet parsing, TinyUSB callbacks, Serial MIDI setup, MIDI instance creation
- Depends on: Adafruit TinyUSB, Arduino MIDI Library, PIO USB
- Used by: Message Router (main sketch)

**Message Router Layer:**
- Purpose: Receives MIDI messages from any interface and forwards to other interfaces after filtering
- Location: `rp2040/rp2040.ino` (lines 280-811)
- Contains: `usbh_on*Handle()` handlers, `usbd_on*()` handlers, C-style wrapper functions
- Depends on: MIDI Transport Layer, Filtering Layer, LED utilities
- Used by: Called by transport layer callbacks

**Filtering Layer:**
- Purpose: Per-interface, per-message-type filtering and per-channel filtering of MIDI messages
- Location: `rp2040/midi_filters.cpp`, `rp2040/midi_filters.h`
- Contains: 2D boolean filter array `[3 interfaces][8 message types]`, 16-element channel enable array
- Depends on: Nothing (standalone module)
- Used by: Message Router, Configuration Layer

**Configuration Layer:**
- Purpose: Persists and manages device settings (filters, channels, IMU config) via EEPROM and JSON
- Location: `rp2040/config.cpp`, `rp2040/config.h`, `rp2040/web_serial_config.cpp`
- Contains: EEPROM read/write, JSON serialization/deserialization, command processing (READALL, SAVEALL, CALIBRATE_IMU)
- Depends on: Filtering Layer, IMU Handler, ArduinoJson, EEPROM
- Used by: Main loop (Web Serial config processing)

**IMU Layer:**
- Purpose: Reads IMU sensor data and converts to MIDI CC messages
- Location: `rp2040/imu_handler.cpp`, `rp2040/imu_handler.h`
- Contains: Sensor initialization, complementary filter, angle-to-MIDI conversion, per-axis routing configuration
- Depends on: FastIMU, Wire (I2C), MIDI Transport Layer
- Used by: Main loop, Configuration Layer

**Utility Layer:**
- Purpose: Cross-cutting concerns (debug output, LED indicators)
- Location: `rp2040/serial_utils.cpp`, `rp2040/led_utils.cpp`
- Contains: Dual-serial print functions (Serial + Serial2), LED trigger/timing logic
- Depends on: Arduino core
- Used by: All other layers

**Web Configurator (separate application):**
- Purpose: Browser-based UI for reading/writing device configuration over Web Serial API
- Location: `web_configurator/`
- Contains: TypeScript app, serial handler, JSON schema validator, HTML UI
- Depends on: Web Serial API, Ajv, Webpack, TypeScript
- Used by: End users via browser

## Data Flow

**MIDI Message Routing (USB Host → other interfaces):**

1. Core 1 calls `USBHost.task()` in `loop1()` → TinyUSB processes USB events
2. `tuh_midi_rx_cb()` fires → reads raw 4-byte USB MIDI packets
3. `processMidiPacket()` parses packet → dispatches to `usbh_on*Handle()` in `rp2040.ino`
4. Handler checks channel filter (`isChannelEnabled()`) → returns if blocked
5. Handler checks message type filter for USB Host (`isMidiFiltered(MIDI_INTERFACE_USB_HOST, ...)`) → returns if blocked
6. Handler forwards to USB Device if connected and not filtered: `USB_D.send*()`
7. Handler forwards to Serial MIDI if not filtered: `sendSerialMidi*()`
8. Handler triggers USB LED indicator

**MIDI Message Routing (Serial → other interfaces):**

1. Core 0 calls `loopSerialMidi()` → `SERIAL_M.read()` processes Serial1 data
2. MIDI library parses bytes → dispatches to `serial_on*()` handlers in `serial_midi_handler.cpp`
3. Handler checks channel filter, then Serial interface filter
4. Forwards to USB Host via `sendNoteOn()`/etc. (raw packet construction in `usb_host_wrapper.cpp`)
5. Forwards to USB Device via `USB_D.send*()` if connected

**Configuration Flow (Web Browser → Device):**

1. Browser opens Web Serial connection at 115200 baud
2. Sends JSON command (`{"command":"READALL"}` or `{"command":"SAVEALL",...}`) over USB CDC Serial
3. `processWebSerialConfig()` in Core 0 loop reads line from `Serial`
4. Deserializes JSON with ArduinoJson → dispatches by command string
5. READALL: `configToJson()` serializes current filters/channels/IMU config → sends JSON response
6. SAVEALL: `updateConfigFromJson()` applies config to in-memory state → schedules delayed EEPROM save (3 seconds)
7. `handleDelayedEEPROMSave()` in main loop performs actual EEPROM write after delay

**IMU → MIDI CC Flow:**

1. `loopIMU()` checks if any axis is enabled → reads IMU sensor data via I2C (Wire1)
2. Complementary filter combines accelerometer and gyroscope data for roll/pitch/yaw angles
3. `angleToMidiCC()` maps angle (within configurable range) to 0-127 MIDI CC value
4. Only sends if value changed from last time (dead-band deduplication)
5. Routes to configured destinations (Serial, USB Device, USB Host) per-axis

**State Management:**
- MIDI filter state: static 2D boolean array in `midi_filters.cpp` (`midiFilters[3][8]`)
- Channel state: static boolean array in `midi_filters.cpp` (`enabledChannels[16]`)
- IMU config: static `IMUConfig` struct in `imu_handler.cpp`
- USB Host state: global variables `midi_dev_addr`, `midi_host_mounted` in `usb_host_wrapper.cpp`
- Connection state: global `isConnectedToComputer` flag in `rp2040.ino`
- All persistent state stored in EEPROM (100 bytes total)

## Key Abstractions

**MIDI Interface Types:**
- Purpose: Enumeration identifying the three MIDI I/O paths
- Defined in: `rp2040/midi_filters.h`
- Values: `MIDI_INTERFACE_SERIAL (0)`, `MIDI_INTERFACE_USB_DEVICE (1)`, `MIDI_INTERFACE_USB_HOST (2)`
- Pattern: Used as array indices into the 2D filter matrix

**MIDI Message Types:**
- Purpose: Enumeration of filterable MIDI message categories
- Defined in: `rp2040/midi_filters.h`
- Values: `MIDI_MSG_NOTE (0)` through `MIDI_MSG_REALTIME (7)`, with `MIDI_MSG_COUNT = 8`
- Pattern: Used as array indices into the 2D filter matrix

**IMUConfig Struct:**
- Purpose: Comprehensive per-axis IMU-to-MIDI mapping configuration
- Defined in: `rp2040/imu_handler.h`
- Pattern: Each axis (roll/pitch/yaw) has: enabled, MIDI channel, CC number, default value, routing flags (toSerial/toUSBDevice/toUSBHost), sensitivity, range

**USB_D (USB Device MIDI):**
- Purpose: MIDI interface for communication with host computer
- Created by: `MIDI_CREATE_INSTANCE()` macro in `rp2040/midi_instances.cpp`
- Type: `midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>>`
- Pattern: Arduino MIDI Library standard interface with callback handlers

**SERIAL_M (Serial MIDI):**
- Purpose: MIDI interface for hardware serial (DIN/TRS MIDI)
- Created by: `MIDI_CREATE_INSTANCE()` macro in `rp2040/serial_midi_handler.cpp`
- Type: `midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>` over Serial1
- Pattern: Arduino MIDI Library standard interface with local callback handlers

## Entry Points

**Firmware (Core 0):**
- Location: `rp2040/rp2040.ino` → `setup()` (line 93), `loop()` (line 192)
- Triggers: Arduino framework boots Core 0 first
- Responsibilities: USB Device descriptors, MIDI handler registration, filter initialization, EEPROM config load, IMU init, Serial MIDI init. Loop processes USB Device MIDI, Serial MIDI, Web Serial config, delayed EEPROM saves, IMU, and LEDs.

**Firmware (Core 1):**
- Location: `rp2040/rp2040.ino` → `setup1()` (line 216), `loop1()` (line 269)
- Triggers: Arduino-pico framework starts Core 1 after Core 0 signals via FIFO
- Responsibilities: PIO USB host initialization, USB host task loop. Core sync via `rp2040.fifo.push()/pop()`.

**Web Configurator:**
- Location: `web_configurator/src/app.ts`
- Triggers: User opens `web_configurator/index.html` in browser
- Responsibilities: DOM event listeners for Connect/Send/Upload/Export/Calibrate buttons, Web Serial communication

## Error Handling

**Strategy:** Defensive checks with debug logging, no exceptions (C++ embedded)

**Patterns:**
- Bounds checking on all filter/channel array access (`if (interface < MIDI_INTERFACE_COUNT && ...)`)
- Return `false` for invalid operations (e.g., `sendMidiPacket()` returns false if not mounted)
- EEPROM layout has fixed size with bounds check before reading IMU config section
- USB device mount timeout (2 seconds) with graceful fallback to standalone mode
- CPU clock validation on Core 1 with infinite halt on invalid frequency
- JSON deserialization error reporting via serial response messages
- IMU initialization returns bool success/failure

## Cross-Cutting Concerns

**Logging:** Dual-serial debug output via `dualPrint()`/`dualPrintln()`/`dualPrintf()` in `rp2040/serial_utils.cpp`. Prints to both USB CDC Serial and hardware Serial2 (GPIO 24/25). Controlled by global `debug` flag (default: true). Very verbose — nearly every MIDI message is logged.

**Validation:** JSON config validation on firmware side via `updateConfigFromJson()` with array size checks. Web configurator uses Ajv JSON Schema validation (`web_configurator/src/validator.ts`).

**Core Synchronization:** Core 0 and Core 1 synchronize startup via `rp2040.fifo.push()/pop()`. Core 0 pushes `0`, waits for `1`; Core 1 waits for `0`, pushes `1`. No other inter-core synchronization exists — USB Host runs independently on Core 1, callbacks fire on Core 1 but handler functions in `rp2040.ino` access shared globals without mutexes.

---

*Architecture analysis: 2026-02-15*
