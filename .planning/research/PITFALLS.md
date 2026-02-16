# Pitfalls Research

**Domain:** RP2040 MIDI router firmware refactoring + CV/Clock output
**Researched:** 2026-02-15
**Confidence:** HIGH (based on codebase analysis, official RP2040/arduino-pico docs, and embedded MIDI domain knowledge)

## Critical Pitfalls

### Pitfall 1: EEPROM Layout Change Bricks Existing Devices

**What goes wrong:**
Adding EEPROM versioning, CV config fields, or reordering the current layout causes existing devices to load garbage data from flash. The current EEPROM has no magic bytes, no version field, and no checksum. On first boot after firmware update, `loadConfigFromEEPROM()` reads 67 bytes of old data at old offsets and interprets them as the new layout. MIDI channels could all be disabled, filter bits inverted, or IMU config corrupt (NaN sensitivity values, channel numbers >16).

**Why it happens:**
The current layout uses sequential `addr++` writes starting at address 0 with no header. Any structural change shifts all subsequent field offsets. The code doesn't validate loaded values against sane ranges. There's an existing 100-byte `CONFIG_EEPROM_SIZE` with only ~33 bytes free. The RP2040 "EEPROM" is actually flash emulation — uninitialized flash reads as `0xFF`, which means booleans load as `true` (all filters blocking, or all channels enabled, depending on interpretation).

**How to avoid:**
1. Add a 4-byte header at EEPROM offset 0: 2 magic bytes (`0x4D 0x50` for "MP") + 1 version byte + 1 checksum byte.
2. On `loadConfigFromEEPROM()`: read header first. If magic bytes don't match, call `resetToDefaults()` and save. If version doesn't match current, run a migration function. If checksum doesn't match, use defaults.
3. Increase `CONFIG_EEPROM_SIZE` to 256 (safe margin for CV config). RP2040 supports up to 4096 bytes.
4. Add range validation after loading each field: clamp MIDI channels to 1-16, CC numbers to 0-127, sensitivity to 0.1-25.5.
5. **Migration path**: Version 0 = current layout (no header). Version 1 = new layout with header. The migration function reads old offsets when version is 0 (no magic bytes detected), converts to new struct, and writes version 1.

**Warning signs:**
- Device boots with all MIDI messages blocked after firmware update
- IMU sends garbage CC values after firmware update
- Web configurator shows all-true or all-false filter states unexpectedly
- `dualPrintf` shows sensitivity values as `0.0` or `25.5` when user set different values

**Phase to address:**
EEPROM refactoring phase — must be done **before** adding any new config fields (CV config, clock config). This is a prerequisite for all feature additions that need persistent storage.

---

### Pitfall 2: Dual-Core Shared State Corruption

**What goes wrong:**
Core 0 reads `midi_host_mounted` and `midi_dev_addr` (written by Core 1) without `volatile` qualifiers or memory barriers. The compiler can cache these in registers on Core 0, meaning Core 0 may not see changes made by Core 1. Worse, the RP2040 Cortex-M0+ cores share SRAM but have no hardware cache coherency protocol — writes from one core may not be immediately visible to the other (though on M0+ this is mainly a compiler optimization issue, not a hardware cache issue).

Concrete failure scenario: USB Host device gets disconnected (Core 1 sets `midi_host_mounted = false`), but Core 0 still has `true` cached in a register. `sendNoteOn()` calls `sendMidiPacket()` which checks `midi_host_mounted` — the stale `true` causes a write to an invalid device address, potentially hanging the USB Host stack or corrupting TinyUSB internal state.

Additionally, `isConnectedToComputer` is set once in `setup()` on Core 0 and read by `serial_midi_handler.cpp` (also Core 0) — this is safe for now. But `midi_host_mounted` is written on Core 1 (`tuh_midi_mount_cb`, `tuh_midi_umount_cb`) and read on Core 0 (every MIDI handler, `imu_handler.cpp` line 255). This is a genuine cross-core data race.

**Why it happens:**
The RP2040 Arduino framework doesn't enforce or warn about missing `volatile`. The current code works by coincidence — debug `dualPrintf` calls act as implicit compiler barriers (they involve function calls the compiler can't optimize across). When debug logging is disabled with `#ifdef DEBUG`, the compiler can aggressively optimize the `midi_host_mounted` check away.

**How to avoid:**
1. Mark all cross-core shared variables `volatile`: `midi_dev_addr`, `midi_dev_idx`, `midi_host_mounted`.
2. For multi-field state changes (mounting sets both `midi_dev_idx` and `midi_host_mounted`), use an RP2040 spin lock from the pico-sdk: `spin_lock_t *lock = spin_lock_init(spin_lock_claim_unused(true))`. Acquire before writing the group of fields, release after.
3. The `midiFilters[][]` and `enabledChannels[]` arrays are written by `updateConfigFromJson()` on Core 0 and read on Core 0 — technically safe, but if MIDI handler callbacks fire from USB Host on Core 1 (they do — `processMidiPacket()` calls `usbh_on*Handle()` from Core 1's `loop1()`), these are also cross-core reads. Mark as `volatile` or protect with a spin lock.
4. Consider atomic flag patterns: `volatile bool` for single-flag checks, spin locks for multi-field state.

**Warning signs:**
- Intermittent "stuck" MIDI routing after USB Host device disconnect/reconnect
- MIDI messages sent to USB Host after physical disconnection
- Removing debug logging causes new "phantom" MIDI behavior
- Hard faults or watchdog resets during rapid USB Host connect/disconnect cycles

**Phase to address:**
Code structure refactoring phase — must be done **before** disabling debug logging (removing implicit compiler barriers) and **before** adding CV output (which adds another cross-core consumer of MIDI state).

---

### Pitfall 3: PWM CV Output Precision Insufficient for 1V/oct

**What goes wrong:**
The RP2040 PWM has a fundamental tradeoff between frequency and resolution. At the default 120MHz CPU clock, the maximum PWM resolution depends on the PWM frequency: `resolution = CPU_clock / (PWM_frequency × prescaler)`. For 1V/oct CV output covering 5 octaves (MIDI notes 36-96, 60 semitones), you need better than 1/60th of the full voltage range per step — roughly 83mV per semitone across 5V = 0.017 duty cycle precision = ~12 bits minimum.

The arduino-pico docs confirm: "At a CPU frequency of 133MHz, the 16 bit maximum range decreases by 1 bit for every doubling of the default PWM frequency of 1 kHz." At 1kHz PWM frequency, you get ~16 bits of resolution — more than enough. But the Sallen-Key low-pass filter in the hardware has fc=9947Hz, meaning the PWM frequency must be well above 10kHz for adequate filtering (ideally >50kHz). At 50kHz, you lose ~6 bits, leaving ~10 bits = 1024 steps for 5V = ~4.9mV per step. For 1V/oct: 83.3mV/semitone ÷ 4.9mV/step = ~17 steps per semitone. That's adequate for semitones but not for pitch bend or fine tuning.

The real trap: using `analogWrite()` with default settings. The default frequency is 1kHz with 8-bit resolution (0-255), giving only 256 steps = 19.6mV/step = ~4 steps per semitone. This is audibly wrong for pitch CV.

**Why it happens:**
Developers use `analogWrite(pin, value)` without calling `analogWriteFreq()` and `analogWriteRange()` first, getting the default 8-bit/1kHz behavior. Or they set a high PWM frequency for better filtering but lose resolution, resulting in audible pitch steps.

**How to avoid:**
1. Use raw pico-sdk PWM API (`pwm_set_wrap()`, `pwm_set_chan_level()`) for maximum control over the frequency/resolution tradeoff.
2. Calculate the optimal PWM parameters: At 120MHz CPU clock, for a target PWM frequency of ~30kHz: `wrap = 120MHz / 30kHz = 4000`. This gives 12-bit effective resolution (4000 steps) at 30kHz — well above the Sallen-Key fc of ~10kHz. 4000 steps across 5V = 1.25mV/step. For 83.3mV/semitone, that's ~67 steps per semitone — excellent.
3. If using `analogWrite()`, explicitly call `analogWriteFreq(30000)` and `analogWriteRange(4000)` before any `analogWrite()` calls.
4. Build a MIDI-note-to-PWM lookup table at compile time (constexpr or const array). Don't compute PWM values at runtime with floating point — the RP2040 Cortex-M0+ has no FPU, and software float introduces rounding errors.
5. Calibrate with a multimeter: measure actual output voltage at C2 (note 36) and C7 (note 96), calculate linear correction.

**Warning signs:**
- Audible pitch steps between semitones (especially in higher octaves)
- Pitch drift when playing the same note repeatedly
- Synthesizer oscillator doesn't track keyboards across octaves
- Output voltage doesn't exactly match expected 1V/oct (e.g., 2.00V at C4, 3.00V at C5)

**Phase to address:**
CV output implementation phase. Must be researched and prototyped **before** committing to final PWM parameters. Hardware filter cutoff frequency constrains the parameter space.

---

### Pitfall 4: EEPROM Write During MIDI Processing Stalls Both Cores

**What goes wrong:**
The RP2040 "EEPROM" is flash emulation. `EEPROM.commit()` erases and writes a 4KB flash sector. During flash erase/write, **Core 1 is automatically paused** by the arduino-pico framework (confirmed in official docs: "When flash erase or write operations are called from core0, core1 will be paused"). This means:

1. USB Host polling stops (Core 1 paused) — any incoming MIDI from the USB Host device is buffered in TinyUSB's FIFO. If the FIFO overflows, MIDI messages are lost.
2. Core 0 is blocked during `EEPROM.commit()` — USB Device MIDI, Serial MIDI, Web Serial, and IMU all stop processing.
3. Flash erase takes ~100-500ms on typical NOR flash. This is an eternity for MIDI real-time messages (MIDI Clock at 120 BPM sends every ~20.8ms).

The existing code mitigates this partially with a 3-second delayed save (`handleDelayedEEPROMSave()`), but the actual commit still blocks when it fires.

**Why it happens:**
Flash-based EEPROM emulation requires erasing a full 4KB sector before writing. This is a hardware limitation of NOR flash — you can't write individual bytes. The RP2040 executes code from the same flash chip, so flash access must be disabled during erase/write, requiring code to run from RAM and interrupts to be disabled.

**How to avoid:**
1. Keep the existing delayed save mechanism (already implemented).
2. Add a "safe to save" check: only commit EEPROM when no MIDI activity has been detected for N milliseconds (e.g., 500ms of silence). Track last MIDI activity timestamp.
3. Consider using LittleFS instead of EEPROM for wear leveling and potentially faster writes (though it still pauses Core 1).
4. Never save EEPROM in response to real-time MIDI data (e.g., don't auto-save on every config change from a MIDI controller).
5. Warn the user via Web Serial response that a brief MIDI interruption will occur during save.
6. For the config save flow: apply config to RAM immediately (already done), schedule EEPROM write for when idle.

**Warning signs:**
- Brief MIDI dropouts or stuck notes after saving config via Web Serial
- MIDI Clock jitter or missed beats when EEPROM save fires during playback
- USB Host device briefly disconnects during save (TinyUSB timeout)

**Phase to address:**
EEPROM refactoring phase. The delayed save mechanism is already in place — enhance it with activity-based gating.

---

### Pitfall 5: Refactoring MIDI Handlers Breaks Cross-Core Call Safety

**What goes wrong:**
The 33+ duplicated handlers are being refactored into a generic `routeMidiMessage()` function. This consolidation seems straightforward but hides a cross-core safety issue: the USB Host MIDI handlers (`usbh_on*Handle()`) are called from `processMidiPacket()` which runs on Core 1 (inside `loop1()` → `usb_host_wrapper_task()` → `USBHost.task()` → `tuh_midi_rx_cb()`). These handlers call `USB_D.sendNoteOn()` (USB Device) and `sendSerialMidiNoteOn()` (Serial MIDI), which operate on Core 0's peripherals.

If the refactored `routeMidiMessage()` function adds any shared mutable state (e.g., a routing table, a message counter, an output buffer), it becomes a cross-core shared resource. If it calls functions that are not reentrant (like `dualPrintf` with its stack-allocated 256-byte buffer), you get stack corruption or garbled output.

**Why it happens:**
The current code "works" because each handler is a standalone function with no shared mutable state beyond the filter arrays (which are read-only during message handling). When refactoring to a single function, it's natural to add local state (rate limiters, last-message deduplication, output routing tables) that becomes shared between cores.

**How to avoid:**
1. Understand which core calls each handler: USB Host handlers run on Core 1, USB Device and Serial handlers run on Core 0.
2. The generic `routeMidiMessage()` must be **reentrant and thread-safe**. No static local variables, no non-atomic shared state.
3. Filter array reads (`isMidiFiltered()`, `isChannelEnabled()`) are safe IF marked `volatile` (read-only during message processing; writes happen from `updateConfigFromJson()` which only runs on Core 0).
4. `USB_D.sendNoteOn()` is called from both cores — verify that the Adafruit TinyUSB `Adafruit_USBD_MIDI` class is safe to call from Core 1. The TinyUSB stack uses mutexes internally, but confirm this.
5. LED trigger functions (`triggerUsbLED()`, `triggerSerialLED()`) use non-atomic `millis()` reads and `digitalWrite()` — these are safe on RP2040 (GPIO writes are atomic at the register level) but the `inLedActive`/`outLedActive` flags should be `volatile`.
6. If adding CV output to the routing path, the CV output function must also be safe to call from either core (or route through a FIFO).

**Warning signs:**
- Garbled debug output (interleaved strings from both cores)
- Intermittent stuck notes or missing MIDI messages after refactoring
- Crashes (hard faults) during high MIDI throughput
- Behavior changes depending on whether debug logging is enabled

**Phase to address:**
Code structure refactoring phase. The handler consolidation must be designed with cross-core safety from the start, not retrofitted.

---

### Pitfall 6: Debug Logging Removal Exposes Timing-Dependent Bugs

**What goes wrong:**
The codebase has 126+ `dualPrint`/`dualPrintf` calls that each block for microseconds to milliseconds (serial write time). Removing these calls with `#ifdef DEBUG` dramatically changes timing:
1. MIDI handlers complete faster, potentially triggering race conditions that were hidden by the logging delay.
2. `loop()` runs much faster, increasing the rate of `USB_D.read()` and `loopSerialMidi()` calls. This could expose buffer overflow bugs in the MIDI library.
3. The `dualPrintf()` function calls act as implicit compiler barriers — removing them lets the compiler optimize more aggressively, potentially reordering memory accesses to shared variables.

The `dualPrintf` to USB CDC Serial is particularly dangerous to remove: it currently acts as a timing spacer between Web Serial config protocol responses. Without it, the Web Serial configurator may receive multiple JSON responses concatenated without newlines.

**Why it happens:**
Debug logging adds deterministic delays that mask race conditions and timing issues. This is the classic "Heisenbug" pattern in embedded systems — the act of observing (logging) changes the behavior.

**How to avoid:**
1. Add `volatile` to all cross-core shared variables **before** removing debug logging (see Pitfall 2).
2. Don't just `#ifdef` out the logging — replace with a proper debug level system. Keep ERROR level always on (to Serial2 only, not USB CDC). Keep INFO level for startup messages. Only remove TRACE/DEBUG level messages.
3. Critically: separate debug output from Web Serial config protocol. Debug goes to Serial2 only. Config JSON goes to Serial (USB CDC) only. This eliminates the CDC contention issue regardless of debug level.
4. After removing debug logging, stress-test with high MIDI throughput (MIDI Clock at 200+ BPM + note streams) for at least 10 minutes to catch timing-dependent failures.
5. Keep the `debug` runtime flag as a Web Serial command (`{"command":"DEBUG","enabled":true}`) for field troubleshooting.

**Warning signs:**
- MIDI routing works perfectly with debug enabled but intermittently fails with debug disabled
- Web Serial configurator gets JSON parse errors after disabling debug
- USB Host device connection detection becomes unreliable
- New hard faults or watchdog resets that didn't occur with logging enabled

**Phase to address:**
Debug infrastructure phase, after cross-core synchronization is fixed. Must not be done simultaneously with handler refactoring — too many variables changing at once.

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Adding new EEPROM fields without versioning | Quick feature addition | Bricks existing devices on firmware update; requires manual factory reset | Never — always version |
| Copy-pasting handler for new interface (CV) | Fast implementation | 44+ handlers instead of 33; exponential maintenance cost | Never — refactor first |
| Using `analogWrite()` defaults for CV | Works in 5 minutes | 8-bit resolution = audibly wrong pitch CV; hard to fix later because API abstraction hides the problem | Only for initial "does the pin output anything" test |
| Storing new config as individual EEPROM bytes | Matches current pattern | Manual address math, no structure, no migration path | Never — switch to struct-based EEPROM storage |
| `delay()` for clock pulse timing | Simple, accurate enough | Blocks MIDI processing; can't do clock + routing simultaneously | Only in standalone clock-only test sketch |
| Running CV output on Core 0 main loop | Simplest architecture | CV update rate coupled to MIDI processing load; jitter in clock output | Acceptable for initial prototype; move to timer interrupt for production |

## Integration Gotchas

Common mistakes when connecting to external services/hardware.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| OPA2335 op-amp CV output | Assuming 3.3V PWM × 1.51x gain = exactly 5V. Real op-amps have offset voltage, gain error, and the RP2040 3.3V rail is not precisely 3.3V | Build a 2-point calibration into firmware: measure actual voltage at note 36 and note 96, store correction factors in EEPROM |
| Eurorack clock output on TRS ring | Using PWM for clock signal — clock should be a clean digital pulse (0V or 5V), not a filtered analog signal | Use `digitalWrite()` for clock, not `analogWrite()`. Clock is a gate signal, not a CV. Consider using a separate GPIO if TRS tip (CV) and ring (clock) are on the same PWM slice |
| EEPROM.commit() during active MIDI | Calling `saveConfigToEEPROM()` while MIDI Clock is running | Gate EEPROM saves on MIDI activity silence; minimum 200ms of no MIDI messages before committing |
| Web Serial config + USB CDC debug | Writing debug messages to same Serial port as config protocol JSON responses | Separate concerns: Serial = JSON config only, Serial2 = debug only. Filter `dualPrint` to only write to Serial2, never to Serial |
| Adding new MIDI interface (CV) to filter array | Incrementing `MIDI_INTERFACE_COUNT` shifts EEPROM offsets and breaks existing stored config | Add CV interface AFTER EEPROM versioning is in place; use version migration to expand the filter array |

## Performance Traps

Patterns that work at small scale but fail under load.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| `dualPrintf()` on every MIDI Clock message | Increasing latency, MIDI Clock jitter, eventually dropped messages | Disable/skip logging for MIDI Clock; log only on start/stop/error | At 120+ BPM (480+ clock msgs/sec), each `dualPrintf` adds ~100us = 48ms/sec wasted |
| Floating-point angle-to-CC conversion on M0+ (no FPU) | Increasing loop time as IMU + MIDI processing compete for CPU | Pre-compute lookup tables; use fixed-point math for IMU angle → CC conversion | When IMU is enabled + high MIDI throughput simultaneously |
| `String` operations in `processWebSerialConfig()` | Heap fragmentation, eventual allocation failure, crash | Use fixed char buffers; avoid Arduino `String` class in embedded firmware | After hundreds of config read/write cycles without reboot |
| Polling I2C for IMU every loop iteration | I2C bus contention adds ~500us per read at 400kHz; blocks MIDI processing | Rate-limit IMU reads (already done at 50ms); consider moving I2C to Core 1 | When adding CV output processing to Core 0 loop alongside IMU |
| PWM duty cycle calculation with `float` division | Each Note-to-CV conversion takes ~20-50us on M0+ software float | Use integer-only lookup table: `const uint16_t noteToCV[128]` computed at compile time | When receiving rapid note sequences (arpeggios, sequences) |

## Embedded-Specific Pitfalls

Domain-specific issues beyond general software development.

| Pitfall | Risk | Prevention |
|---------|------|------------|
| Flash wear from frequent EEPROM saves | RP2040 flash rated for ~100k erase cycles; saving every config change from Web Serial could wear flash in months of heavy use | Debounce saves (already implemented at 3s); add save-count tracking; consider wear-leveled LittleFS |
| Stack overflow on Core 1 (4KB default) | With multicore, each core gets 4KB stack. USB Host + TinyUSB + MIDI packet processing + debug `dualPrintf` (256-byte buffer on stack) can exceed 4KB | Set `bool core1_separate_stack = true;` for 8KB stacks on both cores; reduce `dualPrintf` buffer to 128 bytes or use pre-allocated buffer |
| PWM slice conflict between CV and clock | RP2040 has 8 PWM slices, each controlling 2 pins (even/odd pair). If CV pin and clock pin are on the same slice, they share frequency settings — CV's 30kHz PWM frequency would also apply to clock, making digital pulses invisible at Eurorack input | Choose CV and clock pins on different PWM slices. Even better: use `digitalWrite()` for clock, avoiding PWM entirely |
| Interrupt priority inversion | Adding timer interrupts for clock output while USB Host uses PIO interrupts could starve MIDI processing | RP2040 M0+ has simple fixed-priority NVIC; verify PIO USB interrupts aren't blocked by timer ISR |
| Uninitialized IMUConfig on first boot | Static `IMUConfig` struct starts zeroed, but `setupIMU()` runs before `loadConfigFromEEPROM()` in current code — IMU initializes with zeros, then gets overwritten by EEPROM load | Reorder initialization: load EEPROM config first, then initialize hardware |

## UX Pitfalls

Common user experience mistakes for MIDI/CV hardware.

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No visual feedback during EEPROM save | User presses "Save" in web UI, no confirmation for 3 seconds (delayed save), tries again, triggers double save | Send immediate "config applied" response, then async "saved to flash" response after commit. LED blink on successful save (non-blocking) |
| Factory reset requires re-flashing firmware | User with corrupted config has no recovery path | Implement BOOTSEL-button factory reset: hold BOOTSEL for 5 seconds during boot → reset EEPROM to defaults. Or add `{"command":"FACTORY_RESET"}` to Web Serial protocol |
| CV output calibration requires external tools | User can't verify pitch accuracy without a tuner | Add a "play test scale" Web Serial command that outputs C2-C7 in sequence with 1-second spacing for tuner verification |
| Clock output starts immediately on MIDI Start | Synthesizer receives first clock pulse before it's ready | Send first clock pulse 1ms after Start; this matches MIDI spec timing expectations |
| Config changes take effect in memory but EEPROM save is delayed | User changes config, power cycles device before 3-second save timer fires, loses changes | Warn in Web Serial response: "Config applied. Saving to flash in 3s — do not power off." Consider reducing delay to 1s |

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **MIDI Handler Refactoring:** Often missing reentrant-safety verification — test that the generic router works when called from both Core 0 and Core 1 simultaneously
- [ ] **EEPROM Versioning:** Often missing migration from version 0 (current unversioned layout) — verify that existing devices upgrade cleanly without losing config
- [ ] **CV Output:** Often missing calibration — a PWM duty cycle that's mathematically correct may produce wrong voltage due to op-amp offset, 3.3V rail tolerance, and resistor tolerance in the gain stage
- [ ] **Debug Logging Removal:** Often missing verification that Web Serial config protocol still works without debug messages acting as timing spacers
- [ ] **Clock Output:** Often missing jitter measurement — a clock that "works" may have unacceptable jitter (>1ms) when MIDI processing load is high
- [ ] **SysEx Fix:** Often missing multi-packet reassembly — fixing the 3-byte truncation to handle "large" SysEx (up to 128 bytes) is different from handling "arbitrary" SysEx (firmware dumps up to 64KB)
- [ ] **Channel Filter Fix:** Often missing regression test — fixing `usbd_onControlChange` to add channel filter might inadvertently apply channel filter to USB Device SysEx (which is channel-less)
- [ ] **IMU Sensitivity Storage:** Often missing backwards compatibility — changing from `uint8_t` to `uint16_t` storage doubles the bytes, shifting all subsequent EEPROM fields

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| EEPROM corruption after layout change | LOW | If versioning is in place: firmware auto-detects bad magic/checksum and resets to defaults. If not: user must reflash firmware with factory reset code |
| Cross-core race condition causing stuck notes | MEDIUM | Add `volatile`, verify with stress testing. For deployed devices: power cycle recovers immediately. Long-term: add a "panic reset" — if no MIDI activity for 30s after last note-on, send all-notes-off |
| PWM CV precision too low | HIGH | Requires changing PWM frequency and potentially redesigning the analog filter stage. If using `analogWrite()` defaults, switching to raw pico-sdk PWM API. May need new lookup table calibration |
| EEPROM flash wear-out | HIGH | Replace EEPROM with LittleFS-based config storage with wear leveling. Requires significant code refactoring. Deployed devices with worn flash need board replacement |
| Stack overflow on Core 1 | MEDIUM | Set `core1_separate_stack = true`. If already deployed, firmware update fixes it. Symptoms are intermittent crashes that are hard to diagnose without this knowledge |
| Handler refactoring breaks routing | LOW | Each handler refactoring step should be verified with manual MIDI testing (note on/off through each path). Keep old handlers as reference until all tests pass |

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| EEPROM layout change bricks devices | EEPROM versioning (before any feature additions) | Flash old firmware, save config, flash new firmware, verify config loads correctly or resets cleanly |
| Dual-core shared state corruption | Code structure refactoring (early phase) | Add `volatile` markers, run MIDI stress test with USB Host connect/disconnect cycles during playback |
| PWM CV precision insufficient | CV output implementation (research + prototype sub-phase) | Measure output voltage with multimeter at C2, C3, C4, C5, C6, C7 — must be within ±5mV of expected 1V/oct |
| EEPROM write stalls MIDI | EEPROM refactoring phase | Play MIDI Clock through device while triggering EEPROM save — verify no audible clock jitter |
| Handler refactoring breaks cross-core safety | Code structure refactoring | Stress test: send MIDI from USB Host + USB Device + Serial simultaneously for 5 minutes. No dropped messages, no garbled output |
| Debug removal exposes timing bugs | Debug infrastructure phase (after cross-core fixes) | Disable debug, repeat all MIDI routing tests. Compare behavior debug-on vs debug-off |
| PWM slice conflict CV + clock | CV output implementation (pin assignment sub-task) | Verify CV and clock pins are on different PWM slices by checking RP2040 datasheet table |
| Stack overflow Core 1 | Code structure refactoring (early) | Add `core1_separate_stack = true`, monitor stack usage with watermarking if available |
| Flash wear from EEPROM saves | EEPROM refactoring phase | Add save counter in EEPROM; log warning if >1000 saves |

## Sources

- **arduino-pico EEPROM docs** (v5.5.0): https://arduino-pico.readthedocs.io/en/latest/eeprom.html — Confirms flash emulation, 4KB sector, 256-4096 byte range (HIGH confidence)
- **arduino-pico Multicore docs** (v5.5.0): https://arduino-pico.readthedocs.io/en/latest/multicore.html — Confirms Core 1 paused during flash writes, 4KB stack per core in multicore mode, recommends `volatile` for cross-core state (HIGH confidence)
- **arduino-pico Analog I/O docs** (v5.5.0): https://arduino-pico.readthedocs.io/en/latest/analog.html — Confirms PWM frequency/resolution tradeoff, 1kHz default, 16-bit max range decreasing with frequency (HIGH confidence)
- **Codebase analysis**: Direct inspection of all `.ino`, `.cpp`, `.h` files in `rp2040/` directory (HIGH confidence)
- **Existing concerns documentation**: `.planning/codebase/CONCERNS.md`, `.planning/codebase/ARCHITECTURE.md` (HIGH confidence)
- **RP2040 hardware characteristics**: PWM slice pairing, Cortex-M0+ no-FPU, flash erase timing (MEDIUM confidence — from training data, consistent with official docs patterns)
- **1V/oct CV precision requirements**: Standard Eurorack convention — 83.3mV per semitone, 5V = 5 octaves (HIGH confidence — well-established standard)

---
*Pitfalls research for: RP2040 MIDI router firmware refactoring + CV/Clock output*
*Researched: 2026-02-15*
