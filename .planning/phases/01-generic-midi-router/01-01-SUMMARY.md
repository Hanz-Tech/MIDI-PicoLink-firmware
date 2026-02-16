---
phase: 01-generic-midi-router
plan: 01
subsystem: api
tags: [rp2040, midi, routing, usb, serial, concurrency]

# Dependency graph
requires: []
provides:
  - Generic MIDI routing module (routeMidiMessage) with unified filter/forward logic
  - Mutex-protected USB host state accessor for cross-core safety
affects: [01-generic-midi-router]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Central routing function handles filter + forward + LED"
    - "Mutex-protected cross-core state access"

key-files:
  created:
    - rp2040/midi_router.h
    - rp2040/midi_router.cpp
  modified:
    - rp2040/usb_host_wrapper.h
    - rp2040/usb_host_wrapper.cpp

key-decisions:
  - "Use pico mutex for midi host state pairing rather than critical sections"

patterns-established:
  - "Route via routeMidiMessage() for all interfaces"

# Metrics
duration: 3 min
completed: 2026-02-16
---

# Phase 1 Plan 01: Generic MIDI Router Summary

**Generic routing module with filter-aware forwarding and mutex-protected USB host state reads for cross-core safety.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-16T16:45:34Z
- **Completed:** 2026-02-16T16:49:18Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added mutex-protected, volatile USB host state with a safe accessor for Core 0 reads.
- Created midi_router module with unified routing, filtering, and LED behavior across interfaces.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add cross-core safety to shared variables (ARCH-06)** - `a2ffc4a` (feat)
2. **Task 2: Create midi_router module with routeMidiMessage()** - `89ff0b1` (feat)

**Plan metadata:** _pending_

## Files Created/Modified
- `rp2040/midi_router.h` - Defines MidiMessage struct and routeMidiMessage API.
- `rp2040/midi_router.cpp` - Implements routing logic, filtering, forwarding, and LED triggers.
- `rp2040/usb_host_wrapper.h` - Marks shared host state volatile and adds accessor.
- `rp2040/usb_host_wrapper.cpp` - Adds mutex protection around mount/unmount and accessor implementation.

## Decisions Made
- Use pico mutex (auto_init_mutex) to guard paired host state updates instead of critical sections.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready to rewire handlers to routeMidiMessage in plan 01-02.

---
*Phase: 01-generic-midi-router*
*Completed: 2026-02-16*

## Self-Check: PASSED
