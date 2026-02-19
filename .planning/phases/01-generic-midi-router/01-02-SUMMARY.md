---
phase: 01-generic-midi-router
plan: 02
subsystem: api
tags: [rp2040, midi, routing, usb, serial]

# Dependency graph
requires:
  - phase: 01-generic-midi-router
    provides: midi_router module and cross-core safety primitives
provides:
  - All MIDI handlers are thin wrappers calling routeMidiMessage()
  - Note on/off routing handled via MidiMessage subtype
  - Firmware build verified with arduino-cli
affects: [01-generic-midi-router]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "All interface handlers construct MidiMessage and call routeMidiMessage"

key-files:
  created: []
  modified:
    - rp2040/rp2040.ino
    - rp2040/serial_midi_handler.cpp
    - rp2040/midi_router.h
    - rp2040/midi_router.cpp
    - rp2040/usb_host_wrapper.cpp

key-decisions:
  - "Use MidiMessage subType to distinguish NoteOn/NoteOff in router forwarding"

patterns-established:
  - "Handler bodies are thin wrappers; routing logic centralized in midi_router"

# Metrics
duration: 8 min
completed: 2026-02-16
---

# Phase 1 Plan 02: Handler Rewire Summary

**All USB Host, USB Device, and Serial MIDI handlers now build MidiMessage structs and route through the shared router with note subtype handling.**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-16T16:54:31Z
- **Completed:** 2026-02-16T17:03:06Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Rewired 33+ handlers to thin wrappers calling routeMidiMessage across USB Host/Device and Serial.
- Added MidiMessage subtype support to distinguish NoteOn vs NoteOff routing in the shared router.
- Verified firmware compiles with arduino-cli after refactor.

## Task Commits

Each task was committed atomically:

1. **Task 1: Rewire all handlers in rp2040.ino and serial_midi_handler.cpp** - `d015e8e` (feat)
2. **Task 2: Verify firmware compiles with arduino-cli** - `9848cec` (chore)

**Plan metadata:** _pending_

## Files Created/Modified
- `rp2040/rp2040.ino` - USB Host/Device handlers now construct MidiMessage and call routeMidiMessage.
- `rp2040/serial_midi_handler.cpp` - Serial input handlers now route via MidiMessage wrappers.
- `rp2040/midi_router.h` - Adds MidiMessage subType for NoteOn/NoteOff distinction.
- `rp2040/midi_router.cpp` - Routes NOTE messages based on subType.
- `rp2040/usb_host_wrapper.cpp` - Fix mutex declaration for host state access.

## Decisions Made
- Use MidiMessage subType (0=NoteOn, 1=NoteOff) to select correct forwarding method in router.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fix mutex declaration for auto_init_mutex usage**
- **Found during:** Task 1 (arduino-cli compile)
- **Issue:** usb_host_wrapper.cpp declared `static auto_init_mutex(...)`, which expands to a duplicate static and fails compilation.
- **Fix:** Switched to `auto_init_mutex(midi_host_mutex)` without extra `static`.
- **Files modified:** rp2040/usb_host_wrapper.cpp
- **Verification:** arduino-cli compile succeeded after change
- **Committed in:** d015e8e (Task 1)

**2. [Rule 3 - Blocking] Align volatile declarations for shared USB host state**
- **Found during:** Task 1 (arduino-cli compile)
- **Issue:** Non-volatile extern declarations conflicted with volatile definitions for midi_dev_addr/midi_host_mounted.
- **Fix:** Updated extern declarations to `volatile` in rp2040.ino and serial_midi_handler.cpp.
- **Files modified:** rp2040/rp2040.ino, rp2040/serial_midi_handler.cpp
- **Verification:** arduino-cli compile succeeded
- **Committed in:** d015e8e (Task 1)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were required for successful compilation; no scope change.

## Issues Encountered
- None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 1 complete for handler routing; ready to transition to Phase 2 (module structure).

---
*Phase: 01-generic-midi-router*
*Completed: 2026-02-16*

## Self-Check: PASSED
