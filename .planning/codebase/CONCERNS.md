# Codebase Concerns

**Analysis Date:** 2026-02-15

## Tech Debt

**Massive Code Duplication in MIDI Message Handlers:**
- Issue: Every MIDI message type handler (NoteOn, NoteOff, CC, ProgramChange, PitchBend, Aftertouch, SysEx, Realtime) follows an identical pattern of check-channel → check-source-filter → log → forward-to-other-interfaces-if-not-filtered → trigger-LED. This pattern is copy-pasted across 3 sources (USB Host, USB Device, Serial) × 11+ message types = ~33 nearly identical handler functions.
- Files: `rp2040/rp2040.ino` (lines 284-810), `rp2040/serial_midi_handler.cpp` (lines 119-351)
- Impact: Any change to the routing logic (e.g., adding a new interface, changing filter semantics, adding logging) requires modifying 30+ functions identically. High risk of inconsistency bugs.
- Fix approach: Create a generic `routeMidiMessage()` function that takes source interface, message type, and message data, then performs filtering/routing/LED-triggering in one place. Each handler would become a single-line call to this router.

**`rp2040.ino` is an 811-line monolith:**
- Issue: The main sketch file contains all USB Host and USB Device MIDI handlers (~530 lines of handlers), C-style wrapper functions, and core setup/loop logic. Despite other modules being properly split out, the handlers remain in the main file.
- Files: `rp2040/rp2040.ino`
- Impact: Difficult to navigate and maintain. The handler code should be in its own module similar to how `serial_midi_handler.cpp` was extracted.
- Fix approach: Extract USB Host handlers into `usb_host_midi_handler.cpp` and USB Device handlers into `usb_device_midi_handler.cpp`, or consolidate all routing into a single `midi_router.cpp`.

**Committed 37MB Binary in Repository:**
- Issue: `arduino-cli.exe` (37MB Windows executable) is committed to git, bloating the repository permanently.
- Files: `arduino-cli.exe` (root directory)
- Impact: Every clone downloads 37MB+ unnecessarily. This binary inflates git history permanently even if removed later.
- Fix approach: Remove from git with `git rm`, add to `.gitignore`, and document installation instructions in README instead. Use `git filter-branch` or `git-filter-repo` to purge from history if desired.

**Orphaned Demo/Experiment Directories:**
- Issue: `lsm6ds3/` and `mpu5060_demo/` directories contain standalone Arduino sketches (5 .ino files) from earlier prototyping that are not part of the production firmware.
- Files: `lsm6ds3/fastimu.ino`, `lsm6ds3/mpu_6050_angles.ino`, `lsm6ds3/i2c_scanner/i2c_scanner.ino`, `lsm6ds3/lsm6ds3_angles/lsm6ds3_angles.ino`, `lsm6ds3/SimpleGyroscope/SimpleGyroscope.ino`, `mpu5060_demo/mpu_6050_angles/mpu_6050_angles.ino`
- Impact: Confuses contributors about what code is production vs. experiment. Some sketches use different IMU libraries (Adafruit_MPU6050 vs FastIMU) creating ambiguity.
- Fix approach: Move to a separate `examples/` or `experiments/` directory with a README, or remove entirely if no longer needed.

**Excessive Debug Logging Always Enabled:**
- Issue: `bool debug = true;` is hardcoded in `rp2040/serial_utils.cpp`. Every `dualPrint`/`dualPrintf` call (126+ invocations across the codebase) runs in production. The IMU handler alone prints debug angles every 500ms and every MIDI CC change. Every MIDI message processed generates multiple debug prints.
- Files: `rp2040/serial_utils.cpp` (line 5), `rp2040/rp2040.ino` (throughout handlers), `rp2040/imu_handler.cpp` (lines 92-98, 110-133), `rp2040/config.cpp` (throughout)
- Impact: Serial output bandwidth consumed by debug messages can interfere with Web Serial config protocol (both use USB CDC Serial). Constant `dualPrintf` calls with 256-byte stack buffers add latency to MIDI processing on a real-time system.
- Fix approach: Add compile-time `#ifdef DEBUG` guards or at minimum make `debug` configurable via Web Serial command. Consider different log levels (ERROR, WARN, INFO, DEBUG).

**IMU Config Sensitivity Stored as uint8_t (Lossy Compression):**
- Issue: IMU sensitivity is a `float` (e.g., 1.5) but stored in EEPROM as `(uint8_t)(value * 10)`, limiting precision to 0.1 increments and maximum value of 25.5. The `range` field is stored as `uint8_t`, limiting maximum range to 255 degrees despite the web UI allowing up to 360.
- Files: `rp2040/config.cpp` (lines 51-53, 62-63, 73-74)
- Impact: Configuration values silently truncated. A user setting yaw range to 360° via the web configurator would have it stored as 255° (or wrapped/truncated).
- Fix approach: Use 2-byte storage for range values, or store sensitivity as a fixed-point 16-bit value. Alternatively, clamp UI values to match EEPROM storage limits.

## Known Bugs

**SysEx Messages Truncated to 3 Bytes:**
- Symptoms: SysEx messages longer than 3 bytes are silently dropped when sending to USB Host. Multi-packet SysEx from USB Host is only partially handled.
- Files: `rp2040/usb_host_wrapper.cpp` (lines 152-170 for receive, lines 228-250 for send)
- Trigger: Any SysEx message larger than 3 bytes (common for firmware updates, patch dumps, device-specific control). `sendSysEx()` explicitly returns `false` for messages > 3 bytes (line 249).
- Workaround: None. Large SysEx is silently dropped.

**Missing Channel Filter on USB Device Control Change:**
- Symptoms: CC messages from USB Device are forwarded without checking channel filter.
- Files: `rp2040/rp2040.ino` (line 621-640, `usbd_onControlChange`)
- Trigger: Send a CC message from USB Device on a disabled channel — it passes through while Note messages on the same disabled channel are correctly blocked.
- Workaround: None.

**Polyphonic Aftertouch Not Handled on Serial MIDI Input:**
- Symptoms: Incoming Polyphonic Aftertouch messages received on Serial MIDI are silently discarded.
- Files: `rp2040/serial_midi_handler.cpp` (lines 35-39 comment, no handler registered in `setupSerialMidi()`)
- Trigger: Send Poly AT from a Serial MIDI device — it is never forwarded.
- Workaround: None. The MIDI library may support it via `setHandleAfterTouchPoly` but it was never registered.

**`readLine()` Buffer Accumulation in Web Configurator:**
- Symptoms: Data remaining in `buffer` after extracting a line is discarded on next `readLine()` call because `buffer` is re-initialized to empty string each call.
- Files: `web_configurator/src/serial-handler.ts` (line 60: `let buffer = ""`)
- Trigger: If the device sends two JSON lines in a single read chunk, the second line is lost.
- Workaround: Responses are typically spaced far enough apart. The app reads multiple times in a loop which partially mitigates this.

## Security Considerations

**No Authentication on Web Serial Configuration:**
- Risk: Any application with Web Serial access can read the device's full configuration and overwrite it, including silently disabling MIDI channels or enabling/disabling filters.
- Files: `rp2040/web_serial_config.cpp` (all commands execute without auth)
- Current mitigation: Web Serial API requires user gesture to grant port access (browser-level).
- Recommendations: For a MIDI controller, this is acceptable risk. If security becomes important, add a simple challenge-response or PIN mechanism.

**Adafruit VID/PID Usage:**
- Risk: Using Adafruit's USB VID/PID (`0x239A`, `0x8122`) for a non-Adafruit product may cause driver conflicts or violate USB-IF licensing.
- Files: `rp2040/rp2040.ino` (line 96: `TinyUSBDevice.setID(0x239A, 0x8122)`)
- Current mitigation: None.
- Recommendations: Apply for a USB VID or use a community-shared VID for hobbyist projects. pid.codes offers free PID allocations.

**No Input Validation on EEPROM Load:**
- Risk: Corrupted EEPROM data could load invalid MIDI channel numbers (>16), CC numbers (>127), or sensitivity values producing NaN/Inf. On first boot with uninitialized EEPROM, all filter bytes read as 0xFF (true), blocking all MIDI messages.
- Files: `rp2040/config.cpp` (lines 82-160)
- Current mitigation: `setupMidiFilters()` initializes to all-pass before `loadConfigFromEEPROM()`, but if EEPROM has valid-looking data from a corrupted state, it overwrites with bad values.
- Recommendations: Add a magic byte/checksum at EEPROM start to validate stored data. If invalid, use defaults.

## Performance Bottlenecks

**Debug Printing in MIDI Real-Time Path:**
- Problem: Every MIDI message processed calls `dualPrintf()` which formats a string into a 256-byte stack buffer and writes to two serial ports synchronously. MIDI Clock messages arrive at 24 ppqn × BPM/60 = potentially 480+ messages/second at 120 BPM.
- Files: `rp2040/serial_utils.cpp` (lines 23-37), all MIDI handlers in `rp2040/rp2040.ino` and `rp2040/serial_midi_handler.cpp`
- Cause: Synchronous serial writes block the CPU. Each `dualPrintf` call involves `va_list` processing, `vsnprintf`, and two blocking `Serial.print()` calls. At high MIDI throughput, this adds measurable latency.
- Improvement path: Disable debug output by default. Use compile-time `#ifdef` for debug builds. For MIDI Clock specifically, the commented-out print (line 492 in `rp2040.ino`) shows this was already identified but only partially addressed.

**Blocking `delay()` in `blinkBothLEDs()`:**
- Problem: `blinkBothLEDs(4, 100)` in setup and `blinkBothLEDs(2, 100)` on config save block all MIDI processing for 800ms and 400ms respectively.
- Files: `rp2040/led_utils.cpp` (lines 51-60), `rp2040/rp2040.ino` (line 187), `rp2040/web_serial_config.cpp` (line 57)
- Cause: Synchronous `delay()` calls in the main loop core.
- Improvement path: Use non-blocking LED blink pattern tracked in `handleLEDs()`.

**Blocking `delay(3000)` in IMU Calibration:**
- Problem: `calibrateIMU()` blocks for 3 seconds (delay) + 1 second (100 samples × 10ms delay) = 4 seconds total, during which all MIDI processing halts on Core 0.
- Files: `rp2040/imu_handler.cpp` (lines 186-225)
- Cause: Called from `processWebSerialConfig()` which runs on Core 0.
- Improvement path: Run calibration as a non-blocking state machine, or move to Core 1.

## Fragile Areas

**EEPROM Layout Without Versioning:**
- Files: `rp2040/config.cpp` (lines 7-13)
- Why fragile: The EEPROM layout is defined by sequential `addr++` writes with no version byte or magic header. Adding any new config field shifts all subsequent addresses, corrupting data from previous firmware versions. The comment says "~90 bytes" but `CONFIG_EEPROM_SIZE` is only 100, leaving very little room.
- Safe modification: Add new fields ONLY at the end of the current layout. Never reorder existing fields.
- Test coverage: No automated tests exist.

**Dual-Core Shared State Without Synchronization:**
- Files: `rp2040/rp2040.ino` (line 30: `bool isConnectedToComputer`), `rp2040/usb_host_wrapper.cpp` (lines 9-11: `midi_dev_addr`, `midi_host_mounted`)
- Why fragile: Core 0 reads `isConnectedToComputer` and shared globals while Core 1 runs USB Host tasks that modify `midi_host_mounted` and `midi_dev_addr`. The only `volatile` variable is `core1_booting` (line 45). Access to `midi_host_mounted` from Core 0 (in `imu_handler.cpp` line 255 and throughout `rp2040.ino`) is unprotected.
- Safe modification: Use `volatile` for all cross-core shared variables, or use RP2040 spin locks / critical sections for multi-word state.
- Test coverage: None.

**Web Serial Config Protocol Shares USB CDC With Debug Output:**
- Files: `rp2040/web_serial_config.cpp` (line 16: `Serial.available()`), `rp2040/serial_utils.cpp` (line 10, 34: `Serial.print()`)
- Why fragile: The Web Serial configurator reads JSON commands from `Serial` (USB CDC), but debug output also writes to `Serial`. If debug output happens between the web configurator sending a command and reading the response, the debug text can corrupt JSON parsing on the web side.
- Safe modification: Either disable debug output on `Serial` when web config is active, or use a separate USB CDC interface for debug vs. config.
- Test coverage: None.

## Scaling Limits

**EEPROM Storage:**
- Current capacity: 100 bytes allocated (`CONFIG_EEPROM_SIZE`)
- Limit: Currently using ~67 bytes (24 filters + 16 channels + 27 IMU). Only ~33 bytes remain for new features.
- Scaling path: Increase `CONFIG_EEPROM_SIZE` (RP2040 EEPROM emulation supports up to 4096 bytes). Add version field to enable migration.

**MIDI Interfaces:**
- Current capacity: 3 interfaces (Serial, USB Device, USB Host)
- Limit: Adding a 4th interface (e.g., BLE MIDI) requires changing `MIDI_INTERFACE_COUNT`, adding filter array dimension, updating all handler functions manually due to code duplication.
- Scaling path: Refactor to generic routing table approach.

**Web Configurator Single-Device:**
- Current capacity: Handles exactly one serial port / one device.
- Limit: No multi-device management. `SerialHandler` is a singleton with single reader/writer.
- Scaling path: Not critical for the product's use case.

## Dependencies at Risk

**FastIMU Library:**
- Risk: FastIMU is a community library without guaranteed long-term maintenance. The code at `rp2040/imu_handler.cpp` line 10 uses `MPU6050 IMU(Wire1)` from FastIMU, while demo sketches use Adafruit_MPU6050 library — creating dependency ambiguity.
- Impact: If FastIMU becomes incompatible with newer Arduino-Pico core versions, IMU functionality breaks.
- Migration plan: Could switch to Adafruit_MPU6050 (already used in demo sketches), which has better maintenance guarantees from Adafruit.

**Adafruit TinyUSB:**
- Risk: Low risk, actively maintained. However, the USB Host MIDI API (`tuh_midi_*`) is relatively new and its interface may change.
- Impact: Breaking changes would require updating `rp2040/usb_host_wrapper.cpp`.
- Migration plan: Pin to known-working library version in board manager.

## Missing Critical Features

**No EEPROM Data Integrity Verification:**
- Problem: No magic bytes, version field, or checksum in EEPROM storage.
- Blocks: Safe firmware updates that change config layout. First boot on new hardware loads garbage data.

**No Factory Reset Mechanism:**
- Problem: If EEPROM gets corrupted, there's no way to reset to defaults without reflashing firmware or sending a specific Web Serial command (which doesn't exist).
- Blocks: End-user recovery from bad configurations.

**No Firmware OTA Update:**
- Problem: Firmware updates require physical USB connection and Arduino IDE or similar tool.
- Blocks: Field updates for deployed devices.

## Test Coverage Gaps

**No Automated Tests:**
- What's not tested: The entire firmware has zero automated tests — no unit tests, no integration tests, no CI pipeline.
- Files: All files under `rp2040/`, `web_configurator/src/`
- Risk: Any code change (filter logic, EEPROM layout, IMU calculations, JSON parsing) can introduce regressions undetected. The `angleToMidiCC()` function in `rp2040/imu_handler.cpp` has clamping and mapping math that is particularly susceptible to edge-case bugs.
- Priority: High — at minimum, the filter logic (`midi_filters.cpp`), MIDI routing decisions, `angleToMidiCC()` math, EEPROM serialize/deserialize, and JSON config parsing should have unit tests. The web configurator's `validator.ts` schema validation is testable in isolation.

**Web Configurator Has No Tests:**
- What's not tested: `app.ts` UI logic, `serial-handler.ts` serial communication, `validator.ts` JSON schema validation.
- Files: `web_configurator/src/*.ts`
- Risk: Config validation schema could drift from firmware expectations. The reversed checkbox logic (`!checked` = blocked) is error-prone.
- Priority: Medium — `validator.ts` schema alignment with firmware expectations is the most critical test.

---

*Concerns audit: 2026-02-15*
