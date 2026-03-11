# Phase 3: Non-Blocking Patterns - Research

**Researched:** 2026-02-19
**Domain:** RP2040 firmware timing, non-blocking state machines, dual-core coordination
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None found. (`03-non-blocking-patterns/*-CONTEXT.md` does not exist.)

### Claude's Discretion
None found. (`03-non-blocking-patterns/*-CONTEXT.md` does not exist.)

### Deferred Ideas (OUT OF SCOPE)
None found. (`03-non-blocking-patterns/*-CONTEXT.md` does not exist.)
</user_constraints>

## Summary

Phase 3 is about eliminating blocking delays in the MIDI routing path. The current codebase uses `delay()` in LED blink patterns (`blinkBothLEDs`) and IMU calibration (`calibrateIMU`), which can stall Core 0 for hundreds of milliseconds or multiple seconds. That conflicts with PERF-01/PERF-02 and the stated goal of consistent MIDI routing latency. The code already has a non-blocking pattern for activity LEDs (`handleLEDs` with `millis()` checks), so the next step is to apply the same approach to boot-time LED patterns and IMU calibration.

Two viable implementation patterns exist in this codebase context: (1) convert blocking routines into `millis()`-driven state machines that advance in `loop()` and `loop1()` without delay; or (2) offload the slow IMU calibration to Core 1 with a shared state flag and a non-blocking result handoff. Given Core 1 is currently dedicated to USB Host processing, the safest plan is a non-blocking state machine on Core 0 that samples over time without stalling the loop. If Core 1 offload is used, it must not interfere with USB Host tasks and must be carefully synchronized.

The planning focus should be on: identifying all blocking calls that sit in the MIDI processing loop, providing a consistent non-blocking scheduling API (e.g., `updateLedPatterns()` and `updateImuCalibration()`), ensuring cross-core safety for any shared flags, and defining verification steps that prove no new latency is introduced during animations or calibration.

**Primary recommendation:** Replace blocking LED blink and IMU calibration with `millis()`-based state machines, invoked from the main loop, and keep Core 1 dedicated to USB Host unless explicit performance testing proves safe offload.

## Standard Stack

### Core
| Library/Feature | Version | Purpose | Why Standard |
|---|---|---|---|
| Arduino-Pico | 5.5.0 (project baseline) | RP2040 BSP and timing (`millis()`) | Already in use; provides `millis()` with stable timing for cooperative scheduling. |
| Arduino core timing (`millis()`, `micros()`) | Bundled | Non-blocking time checks | Standard embedded pattern; already used in `handleLEDs()`. |

### Supporting
| Library/Feature | Version | Purpose | When to Use |
|---|---|---|---|
| RP2040 multicore (`setup1()/loop1()`) | Bundled | Optional offload to Core 1 | Only if calibration can be isolated from USB Host tasks and guarded with synchronization. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| `millis()`-based state machines | Timer interrupts | More precise timing but adds ISR complexity and risk to USB Host/PIO timing; not needed for LED/IMU calibration. |
| Core 0 state machine for IMU calibration | Core 1 offload | Offload reduces main loop work but conflicts with current USB Host usage and adds cross-core complexity. |

**Installation:**
```bash
# No new dependencies. Uses existing Arduino-Pico core and timing APIs.
```

## Architecture Patterns

### Recommended Project Structure
```
rp2040/
├── led_utils.h/.cpp        # LED activity + non-blocking blink patterns
├── imu_handler.h/.cpp      # IMU read + non-blocking calibration state
├── rp2040.ino              # Calls update functions in loop()
```

### Pattern 1: Cooperative Time-Slice State Machine
**What:** Replace a blocking routine with a struct that carries state, a `nextStepAt` timestamp, and a step function called every loop.
**When to use:** LED blink patterns and IMU calibration sequences that currently use `delay()`.
**Example (pattern):**
```cpp
// Non-blocking stepper: called from loop()
struct BlinkPattern {
  bool active = false;
  bool ledOn = false;
  uint8_t remaining = 0;
  uint32_t nextStepAt = 0;
  uint16_t intervalMs = 0;
};

void updateBlink(BlinkPattern& p) {
  if (!p.active || millis() < p.nextStepAt) return;
  p.ledOn = !p.ledOn;
  digitalWrite(LED_IN_PIN, p.ledOn ? HIGH : LOW);
  digitalWrite(LED_OUT_PIN, p.ledOn ? HIGH : LOW);
  p.nextStepAt = millis() + p.intervalMs;
  if (!p.ledOn && --p.remaining == 0) p.active = false;
}
```
**Source:** Derived from existing non-blocking LED activity pattern in `rp2040/led_utils.cpp` and required PERF-01.

### Pattern 2: Incremental IMU Calibration
**What:** Spread calibration sampling across loop iterations by taking one sample per tick (or per fixed interval), accumulating sums until complete.
**When to use:** Replace `calibrateIMU()` blocking delays (`delay(3000)` + `delay(10)` loop) to satisfy PERF-02.
**Example (pattern):**
```cpp
enum CalState { CAL_IDLE, CAL_WAIT_STILL, CAL_SAMPLING, CAL_DONE };
struct Calibrator {
  CalState state = CAL_IDLE;
  uint32_t nextStepAt = 0;
  uint16_t samples = 0;
  float gyroXsum = 0, gyroYsum = 0, gyroZsum = 0;
  float accelXsum = 0, accelYsum = 0, accelZsum = 0;
};

void updateCalibration(Calibrator& c) {
  if (millis() < c.nextStepAt) return;
  switch (c.state) {
    case CAL_WAIT_STILL:
      // after 3s, start sampling
      c.state = CAL_SAMPLING;
      break;
    case CAL_SAMPLING:
      // read one sample, accumulate, schedule next
      // when samples reach target, compute offsets and set CAL_DONE
      break;
    default:
      break;
  }
}
```
**Source:** Non-blocking requirements PERF-02 and current blocking routine in `rp2040/imu_handler.cpp`.

### Anti-Patterns to Avoid
 - **Blocking delays in `loop()` or callbacks:** `delay()` in LED blink or calibration stalls routing and breaks PERF-01/PERF-02.
 - **Long-running calibration on Core 1 without coordination:** Core 1 is already used by USB Host tasks; adding heavy work risks USB Host dropouts.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Timing scheduler | Custom timer ISR framework | `millis()`-driven state machines | ISR complexity risks USB Host/PIO timing and isn't needed for LED or calibration.
| Cross-core work queue | Ad-hoc shared flags without `volatile` | Minimal `volatile` flags + loop polling | Existing code already uses cooperative loops; keep it simple and safe. |

**Key insight:** This phase needs predictable behavior, not higher precision. Cooperative `millis()` state machines are sufficient and lower risk.

## Common Pitfalls

### Pitfall 1: Hidden Blocking Paths
**What goes wrong:** `delay()` calls remain in boot flows or calibration helpers, still blocking MIDI routing even after refactor. 
**Why it happens:** Delays are buried in helper functions called from setup or callbacks.
**How to avoid:** `grep` for `delay(` and audit all call sites; ensure no delays run while routing is active.
**Warning signs:** MIDI dropouts during LED blink or calibration; clock jitter under load.

### Pitfall 2: Calibration State Machine Starves Sampling
**What goes wrong:** Calibration never completes because timing logic skips samples or `nextStepAt` is not updated correctly.
**Why it happens:** State machine transitions are not guarded; interval logic is off-by-one.
**How to avoid:** Keep calibration state in a dedicated struct with explicit `state`, `samples`, and `nextStepAt` fields; add debug logs at state transitions (Serial2 only).
**Warning signs:** IMU remains uncalibrated after startup; offsets remain zero.

### Pitfall 3: Cross-Core Shared Flags Not Marked `volatile`
**What goes wrong:** Calibration or LED patterns rely on shared flags but core-to-core updates are not visible.
**Why it happens:** Compiler caching of shared flags; Core 1 and Core 0 are unsynchronized.
**How to avoid:** Mark shared flags `volatile` and keep shared state minimal.
**Warning signs:** Calibration starts but never completes; LED pattern triggers inconsistently.

## Code Examples

Verified patterns from the codebase:

### LED Activity Timing (Existing)
```cpp
// Source: rp2040/led_utils.cpp
if (inLedActive && (millis() - inLedStartMs >= 50)) {
  digitalWrite(LED_IN_PIN, LOW);
  inLedActive = false;
}
```

### Blocking Calibration to Replace
```cpp
// Source: rp2040/imu_handler.cpp
delay(3000);
for (int i = 0; i < calibrationSamples; i++) {
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);
  delay(10);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| `delay()`-based LED blink | `millis()`-based LED activity timers | Already in code | Demonstrates non-blocking timing model to extend. |

**Deprecated/outdated:**
 - `blinkBothLEDs()` blocking pattern in `rp2040/led_utils.cpp` (must be replaced for PERF-01).

## Open Questions

1. **Should IMU calibration run on Core 1?**
   - What we know: Core 1 runs USB Host tasks continuously; calibration currently blocks Core 0.
   - What's unclear: Available Core 1 headroom and USB Host timing sensitivity to extra workload.
   - Recommendation: Default to Core 0 non-blocking state machine; only consider Core 1 after profiling.

2. **When is it safe to run boot-time LED patterns?**
   - What we know: Setup currently blocks with `blinkBothLEDs(4, 100)` after initialization.
   - What's unclear: Whether the device must indicate boot status before MIDI routing begins.
   - Recommendation: Make boot blink non-blocking and allow routing immediately; keep LEDs as background status.

## Sources

### Primary (HIGH confidence)
 - Codebase: `rp2040/led_utils.cpp` (non-blocking activity LEDs, blocking blink pattern)
 - Codebase: `rp2040/imu_handler.cpp` (blocking calibration with `delay()`)
 - Codebase: `rp2040/rp2040.ino` (boot-time blink and `delay()` usage)
 - Project requirements: `.planning/REQUIREMENTS.md` (PERF-01, PERF-02)

### Secondary (MEDIUM confidence)
 - Project research: `.planning/research/ARCHITECTURE.md` (non-blocking state machine recommendation)

### Tertiary (LOW confidence)
 - None

## Metadata

**Confidence breakdown:**
 - Standard stack: HIGH - derived from current codebase and Arduino core usage.
 - Architecture: MEDIUM - patterns based on existing usage and standard embedded practice; no external docs referenced.
 - Pitfalls: MEDIUM - inferred from codebase and known blocking paths.

**Research date:** 2026-02-19
**Valid until:** 2026-03-21
