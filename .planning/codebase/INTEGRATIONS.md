# External Integrations

**Analysis Date:** 2026-02-15

## APIs & External Services

This is an embedded firmware project with a companion web configurator. There are **no cloud APIs, external web services, or third-party SaaS integrations**. All communication is local between hardware and the host computer.

## Hardware Interfaces (On-Device)

### USB Device MIDI
- **Purpose:** Presents the RP2040 as a USB MIDI device to a host computer (DAW, synthesizer software)
- **SDK/Client:** Adafruit TinyUSB Library (`Adafruit_USBD_MIDI`, `Adafruit_TinyUSB.h`)
- **Implementation:** `rp2040/midi_instances.cpp`, `rp2040/midi_instances.h`
- **USB Descriptors:** VID `0x239A`, PID `0x8122`, Product "MIDI PicoLink", Manufacturer "HanzTech"
- **Protocol:** USB MIDI 1.0 over USB CDC
- **Initialization:** `rp2040/rp2040.ino` lines 96-136

### USB Host MIDI
- **Purpose:** Acts as USB Host to receive MIDI from connected USB MIDI devices (keyboards, controllers)
- **SDK/Client:** Adafruit TinyUSB Host + Pico PIO USB (`pio_usb.h`, `Adafruit_USBH_Host`)
- **Implementation:** `rp2040/usb_host_wrapper.cpp`, `rp2040/usb_host_wrapper.h`
- **Hardware:** Bit-banged USB via PIO state machines on GPIO 12 (D+) and GPIO 13 (D-)
- **Protocol:** USB MIDI 1.0 Host mode (raw 4-byte USB MIDI packets)
- **Runs on:** Core 1 exclusively (`loop1()` in `rp2040/rp2040.ino`)

### Serial MIDI (DIN/TRS)
- **Purpose:** Traditional 5-pin DIN or 3.5mm TRS MIDI I/O
- **SDK/Client:** MIDI Library v5.0.2 (`MIDI_CREATE_INSTANCE`)
- **Implementation:** `rp2040/serial_midi_handler.cpp`, `rp2040/serial_midi_handler.h`
- **Hardware:** UART Serial1 on GPIO 0 (TX) and GPIO 1 (RX) at 31250 baud (MIDI standard)
- **Protocol:** Standard MIDI 1.0 serial protocol

### IMU Sensor (I2C)
- **Purpose:** Motion-to-MIDI conversion (roll/pitch/yaw → MIDI CC messages)
- **SDK/Client:** FastIMU library (`FastIMU.h`) — MPU6050 driver
- **Implementation:** `rp2040/imu_handler.cpp`, `rp2040/imu_handler.h`
- **Hardware:** I2C via `Wire1` on GPIO 26 (SDA) and GPIO 27 (SCL) at 400kHz
- **I2C Address:** `0x68` (configurable, supports MPU6050/LSM6DS3)
- **Update Rate:** 20Hz (50ms interval, defined as `IMU_MIDI_UPDATE_RATE_MS`)
- **Processing:** Complementary filter (α=0.98) combining accelerometer + gyroscope data

## Data Storage

### EEPROM (Emulated Flash)
- **Purpose:** Persistent storage of all device configuration (survives power cycles)
- **Client:** Arduino `EEPROM.h` library (flash-backed on RP2040)
- **Implementation:** `rp2040/config.cpp`
- **Size:** 100 bytes allocated (`CONFIG_EEPROM_SIZE`)
- **Layout:**
  - Bytes 0-23: MIDI message type filters (3 interfaces × 8 message types, each 1 byte)
  - Bytes 24-39: Channel enable states (16 channels, each 1 byte)
  - Bytes 40-66: IMU axis configurations (27 bytes: 9 bytes × 3 axes for roll/pitch/yaw)
- **Write Strategy:** Delayed write (3-second delay via `handleDelayedEEPROMSave()` in `rp2040/web_serial_config.cpp`) to avoid interfering with USB Host operations
- **No external database or file storage**

### No File Storage
- No SD card, no filesystem — only EEPROM for persistent data

### No Caching Layer
- All state held in RAM (static arrays in `rp2040/midi_filters.cpp`)

## Communication Protocols

### Web Serial API (Browser ↔ Device)
- **Purpose:** Configure the device from a web browser over USB CDC serial
- **Direction:** Bidirectional JSON commands over USB Serial at 115200 baud
- **Firmware side:** `rp2040/web_serial_config.cpp` — reads JSON from `Serial` (USB CDC)
- **Browser side:** `web_configurator/src/serial-handler.ts` — uses Web Serial API (`navigator.serial`)
- **Protocol:** JSON-over-serial, newline-delimited
- **Commands:**
  - `{"command": "READALL"}` → Returns full config JSON including firmware version
  - `{"command": "SAVEALL", "filters": [...], "channels": [...], "imu": {...}}` → Applies and persists config
  - `{"command": "CALIBRATE_IMU"}` → Triggers IMU calibration sequence
- **Response format:** JSON with `status`, `command`, and optional `message` fields
- **Validation:** Client-side JSON Schema validation using `ajv` (`web_configurator/src/validator.ts`)

### Debug Serial (Serial2)
- **Purpose:** Debug logging output via secondary UART
- **Implementation:** `rp2040/serial_utils.cpp` — `dualPrint()`, `dualPrintln()`, `dualPrintf()` output to both `Serial` (USB CDC) and `Serial2` simultaneously
- **Hardware:** UART Serial2 on GPIO 24 (TX) and GPIO 25 (RX) at 115200 baud
- **Usage:** Debug output only, not for configuration

## Authentication & Identity

- **No authentication** — the device is a USB peripheral, no network access
- **Web Serial API** requires user gesture (port selection dialog) as browser security measure
- **No encryption** on serial communication

## Monitoring & Observability

### Error Tracking
- No external error tracking service
- Errors logged via `dualPrintf()` to USB Serial and debug UART (`rp2040/serial_utils.cpp`)

### Debug Logging
- Dual-output logging to both USB CDC (`Serial`) and hardware UART (`Serial2`)
- Debug flag: `bool debug = true` in `rp2040/serial_utils.cpp` (hardcoded, always on)
- Format: plain text with `[DEBUG]`, `[IMU DEBUG]`, `[IMU MIDI]` prefixes
- Web configurator side: log box in UI shows `[SENT]`, `[RECV]`, `[INFO]`, `[ERROR]` prefixed messages

### LED Indicators
- GPIO 29 (`LED_IN_PIN`): Serial MIDI activity indicator
- GPIO 19 (`LED_OUT_PIN`): USB MIDI activity indicator
- Both blink for 50ms on MIDI event, 4× blink pattern on startup (`rp2040/led_utils.cpp`)

## CI/CD & Deployment

### Firmware Deployment
- **No CI/CD pipeline** detected (no GitHub Actions, no Makefile)
- Manual compilation via Arduino CLI: `./arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040`
- Flash via Arduino IDE or `arduino-cli upload`
- Pre-compiled `arduino-cli.exe` (Windows) is checked into the repo root

### Web Configurator Deployment
- **No hosting configured** — served locally via `http-server` or opened as local file
- Build output: `web_configurator/dist/app.js` (gitignored)
- Can be hosted on any static file server

## Webhooks & Callbacks

### Incoming
- None (no network connectivity)

### Outgoing
- None (no network connectivity)

### TinyUSB Callbacks (Internal)
- USB Host mount/unmount callbacks: `tuh_midi_mount_cb()`, `tuh_midi_umount_cb()` in `rp2040/usb_host_wrapper.cpp`
- USB Host MIDI RX callback: `tuh_midi_rx_cb()` — triggers MIDI packet processing
- USB Device mount detection: `TinyUSBDevice.mounted()` check in `rp2040/rp2040.ino`

## Environment Configuration

### Required Environment Variables
- None — this is an embedded project with no environment variable configuration

### Secrets
- None — no API keys, tokens, or credentials used anywhere

### Hardware Configuration Constants
All hardware configuration is compile-time via `#define` directives:
- `rp2040/rp2040.ino`: `HOST_PIN_DP 12`
- `rp2040/led_utils.h`: `LED_IN_PIN 29`, `LED_OUT_PIN 19`
- `rp2040/imu_handler.h`: `IMU_I2C_SDA_PIN 26`, `IMU_I2C_SCL_PIN 27`, `IMU_ADDRESS 0x68`
- `rp2040/serial_midi_handler.cpp`: `serialRxPin = 1`, `serialTxPin = 0`
- `rp2040/rp2040.ino`: Serial2 RX=25, TX=24

---

*Integration audit: 2026-02-15*
