# Technology Stack

**Analysis Date:** 2026-02-15

## Languages

**Primary:**
- C++ (Arduino dialect) - All firmware code in `rp2040/` directory (`.ino`, `.cpp`, `.h` files)

**Secondary:**
- TypeScript 5.3.3 - Web configurator app in `web_configurator/src/` (3 source files)
- HTML/CSS - Single-page web configurator UI in `web_configurator/index.html`

## Runtime

**Firmware:**
- RP2040 (Raspberry Pi Pico) microcontroller
- Dual-core ARM Cortex-M0+ at 120MHz (required for PIO USB bit-banging)
- CPU must be 120MHz or 240MHz (validated at runtime in `rp2040/rp2040.ino` line 231)
- Core 0: Main MIDI routing, Serial MIDI, Web Serial config, IMU processing
- Core 1: USB Host task (PIO USB)

**Web Configurator:**
- Browser runtime (requires Web Serial API support — Chrome/Edge only)
- Node.js for development tooling (build, dev server)
- Target: ES2020 (configured in `web_configurator/tsconfig.json`)

**Package Manager:**
- npm (lockfile present: `web_configurator/package-lock.json`)

## Frameworks & Platforms

**Arduino Platform:**
- Arduino-Pico (earlephilhower/arduino-pico) v5.5.0 - RP2040 board support
  - Installed via Arduino Board Manager
  - Provides: `Arduino.h`, `HardwareSerial`, `Wire`, `SPI`, `EEPROM`, `pio_usb`
  - USB Stack: **Adafruit TinyUSB** (must be selected in Tools menu)

**Build System:**
- Arduino CLI (`./arduino-cli`) - Firmware compilation
  - Compile command: `./arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040`
  - Board FQBN: `rp2040:rp2040:rpipico`
  - USB stack option: `usbstack=tinyusb`
- Webpack 5.89.0 - Web configurator bundling (`web_configurator/webpack.config.js`)
  - Entry: `./src/app.ts`
  - Output: `dist/app.js`
  - Loader: `ts-loader` 9.5.1

**Web Configurator Build/Dev:**
- TypeScript 5.3.3 - Compilation
- Webpack 5.89.0 - Bundling
- http-server 14.1.1 - Local dev server (port 8080)
- nodemon 3.0.2 - File watching for auto-rebuild
- concurrently 8.2.2 - Running watch + server simultaneously

## Key Dependencies

### Arduino Libraries (Critical)

| Library | Version | Purpose |
|---------|---------|---------|
| Pico PIO USB | 0.7.2 | Bit-banged USB Host on GPIO pins via PIO state machines |
| MIDI Library | 5.0.2 | MIDI message parsing, creation, and routing |
| Adafruit TinyUSB Library | 3.7.4 | USB Device MIDI + USB Host stack. May need manual copy to `Arduino15/packages/rp2040/hardware/rp2040/5.5.0/libraries/` |
| ArduinoJson | 7.4.1 | JSON serialization/deserialization for config commands |
| EEPROM | 1.0 | Persistent storage for MIDI filter/channel/IMU config |
| SPI | 1.0 | Hardware SPI (dependency, not directly used in main code) |
| FastIMU | (bundled) | IMU sensor abstraction for MPU6050/LSM6DS3 (`#include "FastIMU.h"` in `rp2040/imu_handler.cpp`) |
| Wire | (built-in) | I2C communication for IMU sensor (`Wire1` instance) |

### npm Dependencies (Web Configurator)

**Runtime:**
- `ajv` ^8.12.0 - JSON Schema validation for config payloads

**Dev:**
- `@types/ajv` ^1.0.0 - TypeScript types for ajv
- `@types/w3c-web-serial` ^1.0.6 - TypeScript types for Web Serial API
- `concurrently` ^8.2.2 - Run multiple npm scripts
- `http-server` ^14.1.1 - Static file server
- `nodemon` ^3.0.2 - File watcher for auto-rebuild
- `ts-loader` ^9.5.1 - Webpack TypeScript loader
- `typescript` ^5.3.3 - TypeScript compiler
- `webpack` ^5.89.0 - Module bundler
- `webpack-cli` ^5.1.4 - Webpack CLI

## Configuration

**Firmware:**
- No environment files — all config is compile-time (Arduino board settings) or EEPROM-persisted
- USB descriptors set in code: VID `0x239A`, PID `0x8122` (Adafruit), manufacturer "HanzTech", product "MIDI PicoLink"
- GPIO pin assignments hardcoded in headers:
  - `HOST_PIN_DP = 12` (USB Host D+) — `rp2040/rp2040.ino`
  - `LED_IN_PIN = 29`, `LED_OUT_PIN = 19` — `rp2040/led_utils.h`
  - `IMU_I2C_SDA_PIN = 26`, `IMU_I2C_SCL_PIN = 27` — `rp2040/imu_handler.h`
  - Serial MIDI: `RX = GPIO 1`, `TX = GPIO 0` — `rp2040/serial_midi_handler.cpp`
  - Debug Serial2: `RX = GPIO 25`, `TX = GPIO 24` — `rp2040/rp2040.ino`
- EEPROM layout: 100 bytes total starting at address 0 (`rp2040/config.cpp`)
  - Bytes 0-23: MIDI filters (3 interfaces × 8 message types)
  - Bytes 24-39: Channel enables (16 channels)
  - Bytes 40-66: IMU config (27 bytes: 9 bytes × 3 axes)
- Firmware version: `"1.0.0"` defined in `rp2040/version.h`

**Web Configurator:**
- `web_configurator/tsconfig.json` — TypeScript config (strict mode, ES2020 target)
- `web_configurator/webpack.config.js` — Webpack config (production mode, source maps)
- No environment variables needed

**Build Scripts (web_configurator):**
```bash
npm run build        # Webpack production build
npm run build:dev    # Webpack development build
npm run start        # http-server on port 8080
npm run watch        # nodemon watching src/ for .ts changes
npm run dev          # Concurrent watch + start
```

## Platform Requirements

**Development:**
- Arduino IDE or Arduino CLI for firmware compilation
- Arduino-Pico board package v5.5.0 installed
- Required Arduino libraries installed (see table above)
- Node.js + npm for web configurator development
- CPU speed must be set to 120MHz in Arduino IDE

**Production/Deployment:**
- Raspberry Pi Pico (RP2040) hardware
- USB-C connection for USB Device MIDI + Web Serial config
- Optional: IMU sensor (MPU6050 or LSM6DS3) on I2C (Wire1, pins 26/27)
- Optional: Serial MIDI DIN circuit on pins 0/1
- 2× LEDs on GPIO 19 and 29
- Web configurator served as static files (any HTTP server, or local `file://`)
- Web Serial API requires Chrome or Edge browser

---

*Stack analysis: 2026-02-15*
