---
status: investigating
trigger: "it becomes after a couple of minutes, everything becomes non-responseive, the midi device is self powered, the latest commit, no unmount events"
created: 2026-02-16T18:06:00Z
updated: 2026-02-16T18:46:00Z
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: "Core1 stalls inside USBHost.task(), likely due to USB host stack or PIO USB stability with TE OPXY"
test: "Revert diagnostics to baseline (host on core1) to restore original behavior before further tests"
expecting: "Baseline restores USB Host-only stall without freezing core0"
next_action: "ask user to confirm baseline behavior after revert"

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: "USB Host should remain responsive with ongoing LED activity and timely MIDI routing"
actual: "After a couple of minutes, USB Host becomes non-responsive; USB Host LED stops updating; USB Host input becomes delayed; Serial MIDI still works"
errors: "No error messages observed"
reproduction: "USB Host device connected; after a couple of minutes USB Host LED stops and Host input becomes delayed/unresponsive while Serial continues"
started: "Occurs with latest commit; timing ~couple of minutes"

## Eliminated
<!-- APPEND only - prevents re-investigating -->

## Evidence
<!-- APPEND only - facts discovered -->

- timestamp: 2026-02-16T18:08:00Z
  checked: "User report on hardware and behavior"
  found: "Device: TE OPXY, no hub, RP2040 powered via PC USB; issue occurs idle and under traffic; USB Host LED stops; Serial continues"
  implication: "Failure isolated to USB Host stack/device path, not full MCU freeze"

- timestamp: 2026-02-16T18:10:00Z
  checked: "USB host loop and callbacks"
  found: "USB Host runs on core1 loop1() calling usb_host_wrapper_task() → USBHost.task(); callbacks (mount/umount/midi_rx) run in host context and use dualPrintf; mutex used in mount/umount and onMIDIconnect/disconnect"
  implication: "Potential for blocking in host callbacks via mutex or serial printing"

- timestamp: 2026-02-16T18:12:00Z
  checked: "Logging implementation"
  found: "dualPrintf writes to Serial (USB CDC) and Serial2 from any core when debug=true; USB Host callbacks call dualPrintf frequently"
  implication: "Core1 may block on USB CDC or contend with USB device stack, stalling USB Host task"

- timestamp: 2026-02-16T18:14:00Z
  checked: "User test"
  found: "User disabled Serial.print/println in dualPrint but hang still occurs"
  implication: "Partial logging disable insufficient; dualPrintf and/or Serial2 output may still be blocking"

- timestamp: 2026-02-16T18:16:00Z
  checked: "serial_utils.cpp"
  found: "dualPrintf allocates 256-byte stack buffer and uses vsnprintf; enabled by global debug=true"
  implication: "Frequent dualPrintf calls on Core1 could exhaust stack or block, stalling USB Host task"

- timestamp: 2026-02-16T18:18:00Z
  checked: "User test"
  found: "User removed debug output entirely; USB Host still freezes"
  implication: "Logging not the root cause"

- timestamp: 2026-02-16T18:20:00Z
  checked: "Code search"
  found: "No explicit core1 stack config in repo; USBHost.task() is the only work in loop1"
  implication: "If core1 stops, likely stall inside USBHost.task or USB host stack"

- timestamp: 2026-02-16T18:23:00Z
  checked: "Instrumentation"
  found: "Added core1_heartbeat increment in loop1 and 2s Serial2 monitor in loop"
  implication: "Can distinguish core1 stall from USB Host stack wedged while core1 runs"

- timestamp: 2026-02-16T18:26:00Z
  checked: "User test"
  found: "Core1 heartbeat stopped when USB Host froze (HB stuck)"
  implication: "Core1 stalled; likely inside USBHost.task()"

- timestamp: 2026-02-16T18:27:00Z
  checked: "Diagnostic change"
  found: "Moved usb_host_wrapper_task() to core0 loop; core1 now idle with delay(1)"
  implication: "Test will reveal if issue is core1-specific"

- timestamp: 2026-02-16T18:29:00Z
  checked: "User test"
  found: "Core1 heartbeat still stopped with host task on core0"
  implication: "USBHost is still bound to core1 via USBHost.begin(1)"

- timestamp: 2026-02-16T18:30:00Z
  checked: "Diagnostic change"
  found: "Switched USBHost.begin to core0 (begin(0))"
  implication: "Test if core1 stalls stop when host is bound to core0"

- timestamp: 2026-02-16T18:31:00Z
  checked: "User test"
  found: "USB Host still froze and Core1 heartbeat stopped after begin(0)"
  implication: "Core binding change alone did not resolve stall"

- timestamp: 2026-02-16T18:36:00Z
  checked: "Diagnostic refactor"
  found: "Added USB_HOST_ON_CORE0 flag and setupUsbHost() to run full host init/task on core0 with core1 idle"
  implication: "Provides clean A/B test to isolate core-specific behavior"

- timestamp: 2026-02-16T18:39:00Z
  checked: "User test"
  found: "With USB_HOST_ON_CORE0=1, system stops responding for both Serial and USB after last Router log; core1 heartbeat still prints before stall"
  implication: "System-wide stall may be related to logging/Serial I/O or router logging path"

- timestamp: 2026-02-16T18:40:00Z
  checked: "Diagnostic change"
  found: "Removed router logging in midi_router.cpp (dualPrintf)"
  implication: "Test if logging path triggers stall"

- timestamp: 2026-02-16T18:42:00Z
  checked: "User test"
  found: "With USB_HOST_ON_CORE0=1 and router logging removed, entire board freezes; worse than baseline"
  implication: "Core0 host task can stall main loop, turning host stall into full system freeze"

- timestamp: 2026-02-16T18:46:00Z
  checked: "Revert"
  found: "Set USB_HOST_ON_CORE0=0 and restored host task to core1 loop"
  implication: "Back to baseline behavior for further diagnostics"

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: ""
fix: ""
verification: ""
files_changed: []
