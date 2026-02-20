---
phase: 02-module-structure
plan: 01
subsystem: infra
tags: [rp2040, gpio, refactor, pins, firmware]

# Dependency graph
requires:
  - phase: 01-generic-midi-router
    provides: Generic MIDI routing module and cross-core safety baseline
provides:
  - Centralized GPIO pin configuration header (pin_config.h)
  - Modules updated to consume shared pin constants
affects: [02-module-structure]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Centralized pin definitions via pin_config.h"

key-files:
  created:
    - rp2040/pin_config.h
  modified:
    - rp2040/rp2040.ino
    - rp2040/led_utils.h
    - rp2040/imu_handler.h
    - rp2040/serial_midi_handler.cpp

key-decisions:
  - "Use preprocessor constants in pin_config.h for all GPIO assignments"

patterns-established:
  - "Include pin_config.h for any module needing GPIO pin assignments"

# Metrics
duration: 0 min
completed: 2026-02-19
---

# Phase 2 Plan 01: Module Structure Summary

**Centralized all GPIO pin assignments in pin_config.h and replaced module-local pin definitions with shared constants.**

## Performance

- **Duration:** 0 min
- **Started:** 2026-02-19T06:22:53Z
- **Completed:** 2026-02-19T06:23:56Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added a single pin_config.h header defining every hardware GPIO assignment.
- Updated core modules to include pin_config.h and removed duplicated pin definitions.
- Replaced hardcoded Serial1/Serial2 pin literals with centralized constants.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create pin_config.h with all GPIO pin definitions** - `02a02c7` (feat)
2. **Task 2: Update all modules to use pin_config.h and remove local pin definitions** - `bbd1454` (refactor)

**Plan metadata:** _pending_

## Files Created/Modified
- `rp2040/pin_config.h` - Centralized definitions for USB Host, Serial MIDI, LEDs, IMU I2C, and Debug UART pins.
- `rp2040/rp2040.ino` - Uses pin_config.h constants for USB host and debug UART pins.
- `rp2040/led_utils.h` - Pulls LED pin assignments from pin_config.h.
- `rp2040/imu_handler.h` - Pulls IMU I2C pin assignments from pin_config.h.
- `rp2040/serial_midi_handler.cpp` - Uses centralized Serial MIDI pin constants.

## Decisions Made
- Centralize all GPIO pin assignments as preprocessor defines in pin_config.h for clarity and zero runtime overhead.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for module extraction work in plan 02-02.

---
*Phase: 02-module-structure*
*Completed: 2026-02-19*

## Self-Check: PASSED
