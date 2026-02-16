# Requirements: MIDI PicoLink Firmware

**Defined:** 2026-02-15
**Core Value:** Reliable, low-latency MIDI routing between all three interfaces with configurable filtering

## v1 Requirements

Requirements for initial milestone. Each maps to roadmap phases.

### Code Architecture

- [ ] **ARCH-01**: All MIDI message routing consolidated into a single generic `routeMidiMessage()` function that handles filtering, forwarding, and LED triggering for any source interface and message type
- [ ] **ARCH-02**: USB Host and USB Device MIDI handlers extracted from rp2040.ino into dedicated modules (or consolidated into a single midi_router module)
- [ ] **ARCH-03**: All GPIO pin assignments centralized in a single `pin_config.h` header file
- [ ] **ARCH-04**: Compile-time debug logging via `#ifdef DEBUG` macros, outputting to Serial2 only (not USB CDC Serial)
- [ ] **ARCH-05**: Debug serial output separated from Web Serial configuration protocol so they cannot interfere with each other
- [ ] **ARCH-06**: All cross-core shared variables (`midi_host_mounted`, `midi_dev_addr`, `isConnectedToComputer`) use `volatile` qualifier and appropriate synchronization primitives

### Performance

- [ ] **PERF-01**: LED blink patterns use non-blocking state machine driven by `millis()` instead of blocking `delay()` calls
- [ ] **PERF-02**: IMU calibration runs as a non-blocking state machine or on a separate core instead of blocking Core 0 for 4 seconds

## v2 Requirements

Deferred to future milestones. Tracked but not in current roadmap.

### EEPROM & Configuration

- **EEPR-01**: EEPROM versioned config block with magic bytes, version field, and CRC-8 integrity check
- **EEPR-02**: Factory reset mechanism via Web Serial FACTORY_RESET command
- **EEPR-03**: IMU sensitivity and range stored as 16-bit values instead of lossy uint8_t

### Bug Fixes

- **BUGF-01**: USB Device Control Change messages checked against channel filter before forwarding
- **BUGF-02**: Polyphonic Aftertouch handled on Serial MIDI input (register handler in setupSerialMidi())
- **BUGF-03**: Web configurator readLine() buffer persists across calls to avoid losing data
- **BUGF-04**: SysEx messages >3 bytes properly forwarded (multi-packet handling)

### IMU Web Configuration

- **IMUW-01**: IMU CC configuration exposed in web configurator UI (CC number, channel, sensitivity, range per axis)
- **IMUW-02**: IMU recalibration triggerable from web configurator button
- **IMUW-03**: Quick zero-reset for IMU (set current position as zero without full calibration)

### CV Output

- **CVOU-01**: CV pitch output (1V/oct, 0-5V) via RP2040 PWM through Sallen-Key LP and OPA2335 gain stage on TRS tip
- **CVOU-02**: MIDI-to-CV channel selection (which MIDI channel drives CV output)
- **CVOU-03**: CV/Clock configuration added to Web Serial JSON protocol (READALL/SAVEALL)
- **CVOU-04**: CC-to-CV mode (any MIDI CC mapped to 0-5V linear voltage)
- **CVOU-05**: Configurable CV source interface selection

### Clock Output

- **CLKO-01**: Eurorack-style analog clock output (MIDI Clock → pulse on TRS ring)
- **CLKO-02**: Configurable clock division (1/2/4/6/8/12/24 PPQN)
- **CLKO-03**: Configurable clock pulse width (5-50ms)
- **CLKO-04**: Clock source interface selection (which MIDI inputs provide clock)

### Repository Cleanup

- **REPO-01**: Remove committed arduino-cli.exe binary from repository
- **REPO-02**: Clean up orphaned demo directories (lsm6ds3/, mpu5060_demo/)

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Automated test suite | No test infrastructure exists; building it is a separate effort |
| Firmware OTA update | Physical USB flash is acceptable for this product |
| BLE MIDI interface | Adds complexity, not needed for hardware form factor |
| CV input (pitch-to-MIDI) | Requires ADC, pitch detection — different hardware entirely |
| Multiple CV outputs | Hardware has single TRS jack; one CV + one clock is the physical limit |
| Gate output (separate from clock) | Would require additional jack; TRS only has tip + ring |
| Internal clock generator | Device is a MIDI router that receives clock, not generates it |
| Polyphonic/MPE CV | Single CV output, monophonic only |
| Web configurator UI for CV/Clock | JSON protocol in scope; UI deferred to separate milestone |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| ARCH-01 | — | Pending |
| ARCH-02 | — | Pending |
| ARCH-03 | — | Pending |
| ARCH-04 | — | Pending |
| ARCH-05 | — | Pending |
| ARCH-06 | — | Pending |
| PERF-01 | — | Pending |
| PERF-02 | — | Pending |

**Coverage:**
- v1 requirements: 8 total
- Mapped to phases: 0
- Unmapped: 8 ⚠️

---
*Requirements defined: 2026-02-15*
*Last updated: 2026-02-15 after initial definition*
