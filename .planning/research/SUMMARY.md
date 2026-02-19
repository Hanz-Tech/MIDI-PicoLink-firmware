# Research Summary: MIDI PicoLink CV/Clock Milestone

**Domain:** Embedded MIDI Router — CV/Clock Output, EEPROM Versioning, Code Refactoring
**Researched:** 2026-02-15
**Overall confidence:** HIGH

## Executive Summary

The MIDI PicoLink firmware is a working RP2040-based MIDI router that bridges USB Host, USB Device, and Serial MIDI with filtering. This milestone adds CV pitch output (1V/oct) and Eurorack clock output on a shared TRS jack, along with foundational improvements (EEPROM versioning, compile-time debug logging, handler refactoring) that make the new features possible.

The existing codebase has a critical structural problem: 33+ duplicated MIDI handler functions that each copy-paste the same routing logic. Adding CV/Clock as a fourth output destination without first refactoring these handlers would create 44+ duplicated handlers — an unmaintainable mess. The generic `routeMidiMessage()` refactor is therefore a strict prerequisite for the hardware features, not a nice-to-have.

No new libraries are needed. CV output uses the RP2040's built-in PWM hardware via the pico-sdk `hardware/pwm.h` API (bundled with Arduino-Pico 5.5.0). Clock output is a simple GPIO toggle. EEPROM versioning is hand-rolled with a 5-byte header (magic + version + CRC-8). Debug logging switches from runtime flag to compile-time `#define` macros outputting to Serial2 only. The entire milestone builds on the existing Arduino-Pico 5.5.0 + TinyUSB stack with zero new dependencies.

The main risk is the EEPROM layout change: the current sequential byte layout has no versioning, and adding CV/Clock config fields will shift addresses and corrupt existing user configurations. This must be solved before any new config fields are added. The versioned config block (magic bytes + version + CRC-8) enables safe firmware upgrades with automatic migration from the old layout.

## Key Findings

**Stack:** No new libraries needed. Use pico-sdk `hardware/pwm.h` for CV output (not Arduino `analogWrite()`), GPIO for clock. Zero new dependencies.

**Architecture:** CV and clock are output-only sinks fed by a unified `routeMidiMessage()` router function. They are NOT bidirectional MIDI interfaces and should NOT be added to the `MidiInterfaceType` enum or filter matrix.

**Critical pitfall:** EEPROM layout change without versioning will silently corrupt all existing user configs on firmware update. Must implement versioned config block BEFORE adding any new config fields.

## Implications for Roadmap

Based on research, suggested phase structure:

1. **Foundation: Compile-Time Debug Logging** — Quick win (~30 min), immediately improves development experience for everything that follows
   - Addresses: Debug output conflicting with Web Serial, runtime overhead on every MIDI message
   - Avoids: Heisenbug from removing debug logging later (fix cross-core `volatile` first)

2. **Foundation: Generic MIDI Router** — THE key refactor, prerequisite for CV/Clock
   - Addresses: 33+ duplicated handlers → single `routeMidiMessage()` + dispatch table
   - Avoids: Duplicating handler mess for a 4th output destination

3. **Foundation: EEPROM Versioning** — Must exist before adding CV/Clock config fields
   - Addresses: No magic bytes, no version, no checksum in current EEPROM layout
   - Avoids: Bricking existing user configs on firmware update

4. **Foundation: Non-Blocking Patterns** — Required for reliable clock output
   - Addresses: `blinkBothLEDs()` blocks MIDI for 400-800ms
   - Avoids: Clock jitter from blocked main loop

5. **Feature: CV Pitch Output (1V/oct)** — Core hardware feature
   - Addresses: PWM → Sallen-Key LPF → OPA2335 gain → 0-5V on TRS tip
   - Avoids: `analogWrite()` global settings conflict (use pico-sdk PWM API instead)

6. **Feature: Clock Output** — Completes TRS jack functionality
   - Addresses: MIDI Clock (24 PPQN) → configurable divisor → GPIO pulse on TRS ring
   - Avoids: Timer interrupts conflicting with PIO USB (use `millis()` polling instead)

7. **Integration: Web Serial Protocol + Factory Reset** — Makes new features configurable
   - Addresses: CV/Clock config via JSON protocol, factory reset for corrupted EEPROM
   - Avoids: Web configurator UI changes (defer to next milestone)

**Phase ordering rationale:**
- Debug logging first: 30-minute fix that immediately benefits all subsequent development
- Router refactor before everything else: without it, CV/Clock implementation requires duplicating handlers
- EEPROM versioning before CV/Clock: new config fields need versioned storage to avoid corrupting existing configs
- Non-blocking patterns before clock: clock output requires unblocked main loop for timing accuracy
- CV before clock: CV is more complex (PWM configuration, voltage calibration), validates the output-sink architecture pattern
- Web Serial protocol last: depends on CV/Clock implementation existing to expose their config

**Research flags for phases:**
- Phase 2 (Router): Standard refactoring pattern, but needs cross-core safety verification — `routeMidiMessage()` is called from both Core 0 and Core 1
- Phase 5 (CV Output): May need prototyping sub-phase to validate PWM frequency/resolution tradeoff against actual hardware filter response
- Phase 3 (EEPROM): Standard pattern, unlikely to need further research — CRC-8 + magic bytes is well-understood

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All technologies verified against official Arduino-Pico 5.5.0 docs. No new dependencies. pico-sdk PWM API confirmed bundled and mixable with Arduino code. |
| Features | HIGH | Feature list derived from PROJECT.md requirements + codebase analysis. Hardware constraints (single TRS jack, OPA2335 circuit) well-defined. |
| Architecture | HIGH | Router pattern is standard embedded MIDI architecture. Output-sink pattern for CV/Clock avoids filter matrix bloat. Verified against codebase structure. |
| Pitfalls | HIGH | Pitfalls identified from direct codebase inspection (EEPROM layout, duplicated handlers, debug output). Cross-core concerns verified against Arduino-Pico multicore docs. PWM precision calculated from RP2040 clock specs. |

## Gaps to Address

- **CV GPIO pin assignment:** Exact pins depend on PCB routing — not a firmware decision. Any free GPIO works; document PWM slice constraints in `pin_config.h`.
- **CV calibration procedure:** Op-amp offset voltage, 3.3V rail tolerance, and resistor tolerance in the gain stage mean theoretical PWM calculations won't match actual output. Need a 2-point calibration (measure voltage at C2 and C7, compute correction) — design the calibration procedure during CV implementation phase.
- **Core 1 stack size:** USB Host + TinyUSB + MIDI processing may approach the 4KB default Core 1 stack. Monitor during stress testing; set `core1_separate_stack = true` if needed.
- **SysEx passthrough:** Known bug (messages >3 bytes silently dropped) is documented but explicitly deferred to a dedicated bug-fix milestone. Orthogonal to CV/Clock goals.
- **Web configurator UI:** This milestone adds JSON protocol support for CV/Clock config but defers the TypeScript/HTML UI to a follow-up milestone.

## Files Created

| File | Purpose |
|------|---------|
| `.planning/research/SUMMARY.md` | This file — executive summary with roadmap implications |
| `.planning/research/STACK.md` | Technology recommendations: pico-sdk PWM, CRC-8, compile-time macros |
| `.planning/research/FEATURES.md` | Feature landscape: table stakes, differentiators, anti-features, dependencies |
| `.planning/research/ARCHITECTURE.md` | System architecture: router pattern, output sinks, data flow, build order |
| `.planning/research/PITFALLS.md` | Domain pitfalls: EEPROM corruption, PWM precision, cross-core races, timing |

---
*Research summary for: MIDI PicoLink — CV/Clock Milestone*
*Researched: 2026-02-15*
