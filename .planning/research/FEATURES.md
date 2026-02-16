# Feature Landscape

**Domain:** RP2040 MIDI Router — CV/Clock Output, EEPROM Versioning, Code Refactoring
**Researched:** 2026-02-15
**Confidence:** HIGH (features derived from PROJECT.md requirements + codebase analysis)

## Table Stakes

Features the firmware must have for this milestone. Missing = the milestone is incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Generic MIDI routing function | Current 33+ duplicated handlers are unmaintainable; adding CV/Clock without this means duplicating even more code | High | THE key refactor. `routeMidiMessage()` replaces all per-source-per-type handlers. Must preserve existing behavior exactly. |
| EEPROM versioned config block | Current sequential layout has no version, no magic bytes, no checksum. Adding CV/Clock config will shift addresses and corrupt existing user configs on firmware update | Medium | Magic bytes + version byte + CRC-8. Enables safe config migration between firmware versions. |
| Compile-time debug logging | Runtime `debug = true` outputs to USB CDC Serial, conflicting with Web Serial config protocol. Adds latency to every MIDI message. | Low | `#define` macros outputting to Serial2 only. Zero overhead when disabled. |
| CV pitch output (1V/oct) | Core hardware feature. OPA2335 Sallen-Key LP + gain circuit already on PCB. Without CV output, the hardware has unused components. | Medium | PWM → LPF → 0-5V on TRS tip. 12-bit resolution (4096 steps), ~68.5 steps per semitone. Maps MIDI note to voltage. |
| Eurorack clock output | Paired with CV on same TRS jack (ring). Eurorack users expect clock when they have CV. | Low | MIDI Clock (24 PPQN) ÷ configurable divisor → GPIO pulse on TRS ring. Simple counter + non-blocking pulse. |
| MIDI-to-CV channel selection | User must choose which MIDI channel drives CV output. Otherwise all 16 channels fight for control of one CV output. | Low | Single config byte: 1-16 or 0 for omni. Stored in EEPROM ConfigBlock. |
| CV/Clock config in Web Serial protocol | Users configure all other features via Web Serial JSON. CV/Clock without web config would be inconsistent and unusable. | Low | Add `cv` and `clock` JSON keys to READALL/SAVEALL commands. |
| Factory reset mechanism | With versioned EEPROM, corrupted data is detected. Need a way to restore defaults without reflashing. | Low | Web Serial `FACTORY_RESET` command + physical button hold (optional). |
| Non-blocking LED patterns | Current `blinkBothLEDs()` blocks MIDI for 400-800ms. Unacceptable for a MIDI router. Must fix before adding timing-sensitive clock output. | Low | State machine in `handleLEDs()` driven by `millis()`. |

## Differentiators

Features that add value beyond basic CV/Clock. Not strictly required but significantly improve the product.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| CC-to-CV mode | Lets users map any CC (mod wheel, expression, etc.) to CV voltage. Most MIDI-to-CV modules only do note pitch. Expands use cases beyond pitch CV. | Low | Alternate CV mode: CC value 0-127 → 0-5V linear. Same PWM output path, different input mapping. Config flag in EEPROM. |
| Configurable clock division | Different Eurorack modules expect different PPQN. Offering 1/2/4/6/8/12/24 PPQN makes the clock output compatible with more gear. | Low | Single config byte: divisor value. Counter mod divisor triggers pulse. |
| Configurable clock pulse width | Some modules need wider or narrower clock pulses. 5-50ms range covers all common Eurorack modules. | Low | Single config byte: pulse width in ms. |
| Clock source interface selection | Users may want clock from USB Host only (ignoring USB Device clock), or only from Serial MIDI. Interface bitmask lets users select which MIDI input provides clock. | Low | Bitmask config byte: bit 0 = USB Host, bit 1 = USB Device, bit 2 = Serial. |
| Centralized pin config header | All GPIO assignments currently scattered across 5+ files. Centralizing prevents pin conflicts when adding CV/Clock pins and makes PCB rev changes trivial. | Low | `pin_config.h` with all `#define` pin assignments. |
| Volatile/spinlock for cross-core state | Prevents subtle data races between Core 0 and Core 1 that could cause intermittent MIDI routing failures. | Low | Add `volatile` to shared flags, spinlock for multi-word state. |

## Anti-Features

Features to explicitly NOT build in this milestone.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Internal clock generator (BPM-based) | Adds significant complexity (tempo control, start/stop generation, UI for BPM). Not needed when the device is a MIDI *router* that receives clock from external sources. | Clock output only responds to incoming MIDI Clock messages. Internal clock can be a future milestone. |
| CV input (pitch-to-MIDI) | Requires ADC, pitch detection algorithms, and completely different hardware (comparators, sample-and-hold). Out of scope — this is a MIDI-to-CV output device. | Only implement CV *output*. |
| Multiple CV outputs | Hardware has one TRS jack with one tip (CV) and one ring (clock). Can't physically output more than one CV channel without additional hardware. | Single CV + single clock on the one available TRS jack. |
| Gate output (separate from clock) | Would require a third signal on the TRS jack (impossible with stereo TRS) or a separate jack. Hardware constraint. | Use NoteOn/NoteOff to control CV level directly. Clock output on ring doubles as a gate when driven by Note events in a future mode. |
| Web configurator UI changes | This milestone focuses on firmware. The web configurator TypeScript UI can add CV/Clock config panels in a follow-up milestone. The Web Serial JSON protocol must support CV/Clock config, but the UI can come later. | Add JSON protocol support for CV/Clock. Defer UI implementation. |
| Automated test suite | Valuable but significantly expands scope. No test infrastructure exists. Building it from scratch is a separate milestone. | Manual testing with hardware. Document test procedures. |
| Firmware OTA update | Adds massive complexity (bootloader, dual-bank flash, update protocol). Physical USB flash is acceptable. | Continue using USB flash via UF2 bootloader. |
| SysEx passthrough fix | Important bug but complex (requires buffering multi-packet SysEx across USB frames). Orthogonal to CV/Clock goals. | Document as known limitation. Fix in a dedicated bug-fix milestone. |

## Feature Dependencies

```
                    Compile-time debug logging
                              │
                    (enables clean development of everything below)
                              │
                    ┌─────────▼──────────┐
                    │ Generic MIDI Router │
                    │ routeMidiMessage()  │
                    └────────┬───────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
    ┌─────────▼────┐  ┌─────▼──────┐  ┌────▼──────────┐
    │ Non-blocking  │  │ EEPROM     │  │ Centralized   │
    │ LED patterns  │  │ Versioning │  │ pin_config.h  │
    └──────────────┘  └─────┬──────┘  └────┬──────────┘
                            │              │
                     ┌──────▼──────────────▼───┐
                     │  CV Output (PWM 1V/oct) │
                     └──────┬──────────────────┘
                            │
                     ┌──────▼──────────────────┐
                     │  Clock Output (GPIO)    │
                     └──────┬──────────────────┘
                            │
                     ┌──────▼──────────────────┐
                     │  Factory Reset          │
                     └─────────────────────────┘
```

Key dependency chains:
- **Router → CV/Clock:** CV and Clock are output sinks fed by the router. Without the router, you'd have to wire CV/Clock into 33+ duplicated handlers.
- **EEPROM versioning → CV/Clock config:** CV/Clock config needs EEPROM storage. The versioned config block must exist first so that adding new fields doesn't corrupt existing configs.
- **Pin config → CV/Clock:** CV and clock need GPIO pins. Centralizing pin assignments prevents conflicts with existing pins.
- **Debug logging → everything:** Compile-time debug macros should exist before heavy development begins, so developers can debug the router refactor and CV/Clock implementation without polluting USB CDC Serial.
- **Non-blocking LEDs → Clock:** Clock output requires non-blocking main loop. If LEDs block for 800ms, clock pulses are delayed by 800ms.

## MVP Recommendation

### Prioritize (in order):
1. **Compile-time debug logging** — 30 minutes of work, immediately improves development experience for everything else
2. **Generic MIDI router function** — THE foundation. Makes all subsequent features trivial to add. Eliminates 530+ lines of duplicated code.
3. **EEPROM versioned config block** — Required before adding any new config fields. Prevents bricking user configs on firmware update.
4. **Non-blocking LED patterns** — Quick fix, eliminates MIDI dropouts, prerequisite for reliable clock output.
5. **CV pitch output (1V/oct)** — Core hardware feature. Note-to-CV in NOTE_PITCH mode.
6. **Clock output** — Completes the TRS jack functionality. Simple divide-by-N counter.
7. **CV/Clock Web Serial protocol** — JSON config for CV/Clock settings.
8. **Factory reset** — Safety net for corrupted configs.

### Defer to next milestone:
- **CC-to-CV mode:** Nice to have, but NOTE_PITCH mode covers the primary Eurorack use case. Add after basic CV is proven.
- **Web configurator UI for CV/Clock:** Protocol support is in this milestone; the UI can follow.
- **SysEx fix:** Complex, orthogonal, separate milestone.
- **Internal clock generator:** Future feature, not needed for MIDI clock passthrough.
- **Cross-core synchronization improvements:** Important but low-risk in practice (current firmware works). Can be addressed in a quality milestone.

## Sources

- PROJECT.md requirements analysis (2026-02-15)
- Codebase CONCERNS.md analysis (2026-02-15)
- Existing codebase architecture review (20 files in `rp2040/`)
- MIDI 1.0 specification: 24 PPQN clock standard
- Eurorack standards: 1V/oct CV, 5V clock/gate signals
- Hardware constraints from PROJECT.md: OPA2335 Sallen-Key LP + gain, single TRS jack (tip=CV, ring=clock)

---
*Feature landscape for: MIDI PicoLink — CV/Clock Milestone*
*Researched: 2026-02-15*
