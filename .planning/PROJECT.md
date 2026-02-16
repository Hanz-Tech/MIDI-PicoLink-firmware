# MIDI PicoLink Firmware

## What This Is

A USB MIDI router firmware for the RP2040 (Raspberry Pi Pico) that bridges USB Host MIDI, USB Device MIDI, and Serial (DIN/TRS) MIDI with per-interface message filtering, per-channel filtering, IMU-based motion-to-MIDI CC conversion, and a browser-based Web Serial configurator. This is a hardware product by HanzTech targeting musicians and Eurorack users.

## Core Value

Reliable, low-latency MIDI routing between all three interfaces with configurable filtering — if MIDI messages don't get from source to destination correctly, nothing else matters.

## Requirements

### Validated

- ✓ Three-way MIDI routing (USB Host ↔ USB Device ↔ Serial) — existing
- ✓ Per-interface, per-message-type MIDI filtering — existing
- ✓ Per-channel MIDI filtering (16 channels) — existing
- ✓ IMU motion-to-MIDI CC conversion (roll/pitch/yaw) — existing
- ✓ EEPROM-persistent configuration — existing
- ✓ Web Serial JSON configuration protocol (READALL/SAVEALL/CALIBRATE_IMU) — existing
- ✓ Browser-based web configurator UI for filter/channel config — existing
- ✓ Dual-core architecture (Core 0: routing/config, Core 1: USB Host) — existing
- ✓ LED activity indicators — existing

### Active

- [ ] Refactor MIDI handlers into generic routing function (eliminate 33+ duplicated handlers)
- [ ] Split rp2040.ino monolith into focused modules
- [ ] Add EEPROM versioning and data integrity verification (magic bytes, checksum)
- [ ] Add factory reset mechanism
- [ ] Fix SysEx truncation (messages >3 bytes silently dropped)
- [ ] Fix missing channel filter on USB Device Control Change
- [ ] Fix Polyphonic Aftertouch not handled on Serial MIDI input
- [ ] Fix web configurator readLine() buffer accumulation bug
- [ ] Replace blocking delays with non-blocking patterns (LEDs, IMU calibration)
- [ ] Add compile-time debug logging control (#ifdef DEBUG)
- [ ] Fix IMU config lossy uint8_t EEPROM storage (sensitivity/range truncation)
- [ ] Add dual-core shared state synchronization (volatile/spinlocks)
- [ ] Separate debug serial output from Web Serial config protocol
- [ ] Remove committed arduino-cli.exe binary, add to .gitignore
- [ ] Clean up orphaned demo directories (lsm6ds3/, mpu5060_demo/)
- [ ] Expose IMU CC configuration in web configurator UI (CC number, channel, sensitivity, range per axis)
- [ ] Add IMU recalibration button in web configurator UI
- [ ] Add quick zero-reset for IMU (set current position as zero without full calibration)
- [ ] CV output via PWM → OPA2335 Sallen-Key LP → 1V/oct pitch CV (0-5V, tip of TRS)
- [ ] Eurorack-style analog clock output (ring of TRS)
- [ ] Configurable MIDI-to-CV mapping (Note-to-CV pitch or CC-to-CV, per output)

### Out of Scope

- Firmware OTA update — physical USB flash is acceptable for this product
- Multi-device web configurator — single device at a time is sufficient
- BLE MIDI — adds complexity, not needed for the hardware form factor
- USB VID/PID registration — acknowledged concern, not blocking for development
- Automated test suite — valuable but not in scope for this milestone

## Context

- Brownfield project: working firmware exists with ~811-line main sketch and web configurator
- Codebase map completed (`.planning/codebase/`) documenting architecture, stack, concerns
- Hardware: RP2040 Pico, OPA2335AIDR dual op-amp for CV output
  - Op-amp A: Sallen-Key low-pass filter, fc=9947Hz, Q=0.5, damping ratio=1
  - Op-amp B: Non-inverting amplifier, gain=1.51x (3.3V PWM → ~5V CV)
  - Both rail-to-rail
- CV/Clock output shares a single 3.5mm stereo TRS jack: CV on tip, clock on ring
- IMU: MPU6050 or LSM6DS3 via I2C (Wire1), using FastIMU library
- Web configurator: TypeScript + Webpack, uses Web Serial API (Chrome/Edge only)
- Arduino-pico board package v5.5.0, Adafruit TinyUSB stack
- Debug logging currently always on, interferes with Web Serial protocol on shared USB CDC

## Constraints

- **Hardware:** RP2040 dual-core Cortex-M0+ at 120MHz — limited compute, no FPU
- **USB Stack:** Must use Adafruit TinyUSB (required for USB Host + Device simultaneously)
- **EEPROM:** Currently 100 bytes allocated, RP2040 supports up to 4096 bytes emulated
- **Real-time:** MIDI routing must remain low-latency — no blocking operations in message path
- **CV Precision:** 1V/oct requires accurate PWM duty cycle mapping; RP2040 PWM is 12-bit at best resolution
- **GPIO:** Pin allocation constrained by existing interfaces (USB Host PIO on GPIO 12/13, I2C on 26/27, Serial MIDI on 0/1, LEDs on 19/29, Debug UART on 24/25)
- **Browser:** Web configurator requires Web Serial API (Chrome/Edge only)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Refactor before bug fixes | Clean code structure makes bug fixes easier and less error-prone | — Pending |
| PWM-based CV output with OPA2335 | Hardware already designed with Sallen-Key LP + non-inverting gain stage | — Pending |
| Single TRS jack for CV+Clock | CV on tip, clock on ring — matches common Eurorack patch cable use | — Pending |
| Keep FastIMU library | Already integrated, switching to Adafruit_MPU6050 adds risk for minimal gain | — Pending |
| Generic MIDI router function | Eliminates 33+ duplicated handlers, makes adding interfaces (CV) straightforward | — Pending |

---
*Last updated: 2026-02-15 after initialization*
