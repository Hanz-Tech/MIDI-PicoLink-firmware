# Roadmap: MIDI PicoLink — Code Refactoring & Performance

## Overview

This milestone transforms the 811-line monolithic firmware into a modular, maintainable architecture with a generic MIDI routing function, proper debug infrastructure, cross-core safety, and non-blocking timing patterns. These foundational improvements are prerequisites for the CV/Clock output features planned in the next milestone — without them, adding a fourth output destination would expand 33+ duplicated handlers to 44+.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Generic MIDI Router** - Consolidate 33+ duplicated handlers into single routing function with cross-core safety
- [ ] **Phase 2: Module Structure** - Split monolith into focused modules with centralized pin configuration
- [x] **Phase 3: Non-Blocking Patterns** - Replace blocking delays with state machines for LEDs and IMU calibration
- [ ] **Phase 4: Debug Infrastructure** - Separate debug logging from Web Serial and add compile-time control

## Phase Details

### Phase 1: Generic MIDI Router
**Goal**: All MIDI message routing flows through a single `routeMidiMessage()` function with proper cross-core synchronization, eliminating handler duplication and making it safe to add new output destinations
**Depends on**: Nothing (first phase)
**Requirements**: ARCH-01, ARCH-06
**Success Criteria** (what must be TRUE):
  1. A single `routeMidiMessage()` function handles filtering, forwarding, and LED triggering for all message types from all three source interfaces
  2. No duplicated routing logic exists — each MIDI handler calls `routeMidiMessage()` with message type, source, and data
  3. Cross-core shared variables (`midi_host_mounted`, `midi_dev_addr`, `isConnectedToComputer`) are marked `volatile` and access is synchronized with appropriate primitives (spinlocks or mutexes)
  4. MIDI messages route correctly between all three interfaces with the same filtering behavior as before the refactor
**Plans:** 2 plans

Plans:
- [ ] 01-01-PLAN.md — Create midi_router module with routeMidiMessage() + cross-core safety (ARCH-06)
- [ ] 01-02-PLAN.md — Rewire all 33+ handlers to use routeMidiMessage() + verify compilation (ARCH-01)

### Phase 2: Module Structure
**Goal**: The monolithic rp2040.ino is split into focused modules with all hardware pin assignments in one place, creating clear homes for the router refactor and future features
**Depends on**: Phase 1
**Requirements**: ARCH-02, ARCH-03
**Success Criteria** (what must be TRUE):
  1. All GPIO pin assignments (USB Host, I2C, Serial MIDI, LEDs, Debug UART) are defined in a single `pin_config.h` header — no magic numbers scattered in source files
  2. USB Host MIDI handlers live in a dedicated module separate from rp2040.ino
  3. USB Device MIDI handlers live in a dedicated module separate from rp2040.ino (or consolidated with Host handlers in a single midi module)
  4. Firmware compiles and all three MIDI interfaces (USB Host, USB Device, Serial) route messages identically to before the split
**Plans:** 2 plans

Plans:
- [ ] 02-01-PLAN.md — Create pin_config.h and centralize all GPIO pin definitions (ARCH-03)
- [ ] 02-02-PLAN.md — Extract USB Host + Device handlers into dedicated modules, slim rp2040.ino (ARCH-02)

### Phase 3: Non-Blocking Patterns
**Goal**: The main loop never blocks on LED animations or IMU calibration, ensuring consistent MIDI routing latency and preparing for timing-critical clock output in the next milestone
**Depends on**: Phase 2
**Requirements**: PERF-01, PERF-02
**Success Criteria** (what must be TRUE):
  1. LED blink patterns (USB host connect/disconnect, activity indicators) use a `millis()`-based state machine — no `delay()` calls in the MIDI routing path
  2. IMU calibration runs as a non-blocking state machine (or is offloaded to Core 1) so MIDI routing on Core 0 continues uninterrupted during the ~4-second calibration
  3. MIDI messages arriving during LED animations or IMU calibration are routed without added latency (no blocking waits in the message path)
**Plans**: 2 plans

Plans:
- [x] 03-01-PLAN.md — Replace blocking LED blink with non-blocking scheduler (PERF-01)
- [x] 03-02-PLAN.md — Convert IMU calibration to non-blocking state machine (PERF-02)

### Phase 4: Debug Infrastructure
**Goal**: Debug output flows through a dedicated serial channel with compile-time on/off control, eliminating interference with the Web Serial configuration protocol
**Depends on**: Phase 3
**Requirements**: ARCH-04, ARCH-05
**Success Criteria** (what must be TRUE):
  1. Debug log messages appear only on Serial2 (UART TX pin 24), never on USB CDC Serial
  2. Web Serial configurator can send READALL/SAVEALL commands without debug text corrupting JSON responses
  3. Building with `#define DEBUG` disabled produces a binary with zero debug string overhead — no Serial print calls compiled in
**Plans**: TBD

Plans:
- [ ] 04-01: TBD
- [ ] 04-02: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Generic MIDI Router | 0/2 | Planned | - |
| 2. Module Structure | 2/2 | Complete | 2026-02-20 |
| 3. Non-Blocking Patterns | 2/2 | Complete | 2026-02-23 |
| 4. Debug Infrastructure | 0/0 | Not started | - |
