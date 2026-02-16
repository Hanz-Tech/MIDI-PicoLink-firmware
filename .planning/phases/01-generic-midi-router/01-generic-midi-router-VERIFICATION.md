---
phase: 01-generic-midi-router
verified: 2026-02-16T18:05:00Z
status: gaps_found
score: 3/5 must-haves verified
gaps:
  - truth: "All MIDI message routing flows through routeMidiMessage() (no direct multi-interface routing elsewhere)"
    status: failed
    reason: "IMU-generated MIDI CCs are routed directly to Serial/USB Device/USB Host without using routeMidiMessage()."
    artifacts:
      - path: "rp2040/imu_handler.cpp"
        issue: "sendIMUMidiCC() calls sendSerialMidiControlChange/USB_D.sendControlChange/sendControlChange directly."
    missing:
      - "Route IMU-generated MIDI messages through routeMidiMessage() (or document and refactor to use the generic router)."
human_verification:
  - test: "Compile firmware with arduino-cli"
    expected: "./arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040 completes successfully"
    why_human: "Build not executed during verification; requires local toolchain."
  - test: "End-to-end routing between USB Host, USB Device, and Serial"
    expected: "Messages received on any interface are filtered and forwarded to the other two, with LED behavior preserved and Clock LED suppressed on USB Device source."
    why_human: "Runtime behavior and hardware IO cannot be verified statically."
---

# Phase 01: Generic MIDI Router Verification Report

**Phase Goal:** All MIDI message routing flows through a single routeMidiMessage() function with proper cross-core synchronization, eliminating handler duplication and making it safe to add new output destinations
**Verified:** 2026-02-16T18:05:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | A single routeMidiMessage() function handles channel filtering, source filtering, forwarding, and LED triggers | ✓ VERIFIED | `rp2040/midi_router.cpp` includes channel filter (`isChannelEnabled`), source filter (`isMidiFiltered`), forwarding loop, and LED triggers. |
| 2 | All USB Host/USB Device handlers in `rp2040.ino` are thin wrappers that construct MidiMessage and call routeMidiMessage() | ✓ VERIFIED | `rp2040.ino` handlers at lines 285-485 show only MidiMessage construction + `routeMidiMessage(...)`. |
| 3 | All Serial MIDI handlers in `serial_midi_handler.cpp` are thin wrappers calling routeMidiMessage() | ✓ VERIFIED | `serial_midi_handler.cpp` lines 117-209 show MidiMessage construction + `routeMidiMessage(...)`. |
| 4 | Cross-core shared USB host state is synchronized (volatile + mutex-protected paired updates + safe accessor) | ✓ VERIFIED | `usb_host_wrapper.cpp` uses `volatile`, `auto_init_mutex`, and `getMidiHostState()` with mutex. `usb_host_wrapper.h` exports volatile vars and accessor. |
| 5 | All MIDI message routing flows through routeMidiMessage() (no direct multi-interface routing elsewhere) | ✗ FAILED | `rp2040/imu_handler.cpp` routes directly to Serial/USB Device/USB Host in `sendIMUMidiCC()` without using routeMidiMessage(). |

**Score:** 3/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `rp2040/midi_router.h` | MidiMessage struct + routeMidiMessage declaration | ✓ VERIFIED | Contains `MidiMessage` fields and `routeMidiMessage` declaration. |
| `rp2040/midi_router.cpp` | Router implementation with filters/forwarding/LED | ✓ VERIFIED | Implements filter checks, forwarding to all interfaces, LED triggers, and Clock LED suppression. |
| `rp2040/rp2040.ino` | USB Host/Device handlers call routeMidiMessage | ✓ VERIFIED | All handlers construct MidiMessage and call router. |
| `rp2040/serial_midi_handler.cpp` | Serial handlers call routeMidiMessage | ✓ VERIFIED | Local handlers build MidiMessage and call router. |
| `rp2040/usb_host_wrapper.cpp`/`.h` | Cross-core state is volatile + mutex + accessor | ✓ VERIFIED | Volatile vars + `auto_init_mutex` + `getMidiHostState()` present. |
| `rp2040/imu_handler.cpp` | No direct multi-interface routing outside router | ✗ FAILED | `sendIMUMidiCC()` calls direct send functions to three interfaces. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `rp2040/midi_router.cpp` | `rp2040/midi_filters.h` | `isMidiFiltered`/`isChannelEnabled` | ✓ WIRED | Filter calls present. |
| `rp2040/midi_router.cpp` | `rp2040/usb_host_wrapper.h` | `sendNoteOn/Off/RealTime` etc. | ✓ WIRED | Forwarding uses USB Host send helpers. |
| `rp2040/midi_router.cpp` | `rp2040/serial_midi_handler.h` | `sendSerialMidi*` | ✓ WIRED | Forwarding uses serial send helpers. |
| `rp2040/midi_router.cpp` | `rp2040/midi_instances.h` | `USB_D.send*` | ✓ WIRED | USB Device forwarding uses `USB_D` methods. |
| `rp2040/rp2040.ino` | `rp2040/midi_router.h` | include + calls | ✓ WIRED | Includes header and calls router in handlers. |
| `rp2040/serial_midi_handler.cpp` | `rp2040/midi_router.h` | include + calls | ✓ WIRED | Includes header and calls router in handlers. |
| `rp2040/imu_handler.cpp` | `rp2040/midi_router.h` | routeMidiMessage usage | ✗ NOT_WIRED | Direct send calls bypass router. |

### Requirements Coverage

| Requirement | Status | Blocking Issue |
| --- | --- | --- |
| ARCH-01 | ✗ BLOCKED | Direct routing in `imu_handler.cpp` bypasses `routeMidiMessage()`; goal not fully enforced globally. |
| ARCH-06 | ✓ SATISFIED | Volatile + mutex-protected host state updates and safe accessor present. |

### Anti-Patterns Found

No TODO/FIXME/placeholder stubs detected in router or handlers.

### Human Verification Required

1. **Compile firmware with arduino-cli**
   - **Test:** `./arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040`
   - **Expected:** Build completes successfully
   - **Why human:** Build toolchain not run during verification

2. **End-to-end routing behavior**
   - **Test:** Send MIDI messages from each interface (USB Host, USB Device, Serial)
   - **Expected:** Messages are filtered and forwarded to the other two interfaces; LED behavior preserved and USB Device Clock LED suppressed
   - **Why human:** Requires hardware IO and runtime observation

### Gaps Summary

The core router is implemented and all MIDI handlers in `rp2040.ino` and `serial_midi_handler.cpp` now flow through `routeMidiMessage()`. Cross-core synchronization for USB host state is present. However, IMU-generated MIDI CC messages are still routed directly to each output interface in `imu_handler.cpp`, which violates the phase goal that all routing flows through the single router and undermines the “safe to add new output destinations” requirement. This should be refactored to route IMU output through `routeMidiMessage()` or explicitly excluded with rationale.

---

_Verified: 2026-02-16T18:05:00Z_
_Verifier: Claude (gsd-verifier)_
