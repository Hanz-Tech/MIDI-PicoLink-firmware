---
status: investigating
trigger: "it becomes after a couple of minutes, everything becomes non-responseive, the midi device is self powered, the latest commit, no unmount events"
created: 2026-02-16T18:06:00Z
updated: 2026-02-18T12:20:00Z
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: "PIO USB host init/timing or PIO selection affects enumeration; host may miss initial connect without a reset"
test: "Confirm mount success across boot sequence and review PIO init order (configure_pio_usb + begin)"
expecting: "If PIO init/timing is the issue, mount LED will be inconsistent or missing on cold boot but more reliable after replug"
next_action: "Verify mount LED behavior for pre-connected vs plug-after-boot; log if USBHost.configure_pio_usb and USBHost.begin run before device attach"

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: "USB Host should remain responsive with ongoing LED activity and timely MIDI routing"
actual: "After a couple of minutes, USB Host becomes non-responsive; USB Host LED stops updating; USB Host input becomes delayed; Serial MIDI still works"
errors: "No error messages observed"
reproduction: "USB Host device connected; after a couple of minutes USB Host LED stops and Host input becomes delayed/unresponsive while Serial continues"
started: "Occurs with latest commit; timing ~couple of minutes"

## Eliminated
<!-- APPEND only - prevents re-investigating -->

- hypothesis: "CDC Serial.print() in dualPrintf blocks Core1 when no serial monitor is connected"
  evidence: "Applied if(Serial) guards; user reports USB Host MIDI still does not trigger LED when serial monitor is closed"
  timestamp: 2026-02-18T12:20:00Z

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

- timestamp: 2026-02-16T18:52:00Z
  checked: "Reference example (rppicomidi usb_midi_host_pio_example.ino)"
  found: "Example uses tuh_midi_stream_read/write + tuh_midi_write_flush in loop; sets optional buffer limits; uses VBUS enable pin; runs USBHost.task() in loop; prints via UART0"
  implication: "Current implementation uses tuh_midi_packet_read/write without periodic flush or stream handling; may behave differently under load"

- timestamp: 2026-02-16T18:58:00Z
  checked: "Implementation changes"
  found: "Switched RX to tuh_midi_stream_read and TX to tuh_midi_stream_write + flush in usb_host_wrapper.cpp"
  implication: "Aligns host implementation with reference example for stability testing"

- timestamp: 2026-02-16T19:03:00Z
  checked: "Build error"
  found: "tuh_midih_define_limits not available in installed Adafruit_TinyUSB_Library"
  implication: "Removed buffer limit call; proceed with stream API changes only"

- timestamp: 2026-02-16T19:07:00Z
  checked: "User test"
  found: "USB Host can receive messages from Serial (TX works), but RP2040 no longer receives MIDI from USB Host"
  implication: "RX path likely broken after stream API change"

- timestamp: 2026-02-16T19:10:00Z
  checked: "Diagnostic change"
  found: "Reverted RX callback to tuh_midi_packet_read; TX remains stream write + flush"
  implication: "Should restore USB Host input if stream RX parsing was the issue"

- timestamp: 2026-02-17T10:00:00Z
  checked: "TinyUSB MIDI Device write path (midi_device.c)"
  found: "tud_midi_n_stream_write() has inner while loop: while (i < bufsize && tu_fifo_remaining(&midi->tx_ff) >= 4). Assembles 4-byte USB-MIDI packets into tx_ff FIFO (64 bytes = 16 packets max). After each complete packet, calls write_flush() which calls usbd_edpt_claim(). FIFO has mutex (tx_ff_mutex via CFG_FIFO_MUTEX) for cross-core writes."
  implication: "If tx_ff fills up (16 packets), the while loop condition fails and the function returns having written only partial data. But with repeated rapid calls from Core1, the FIFO drains only when write_flush successfully claims endpoint AND Core0's tud_task_ext processes the completion event."

- timestamp: 2026-02-17T10:05:00Z
  checked: "tu_edpt_claim() in tusb.c (lines 208-223)"
  found: "tu_edpt_claim does pre-check (busy==0 && claimed==0) BEFORE taking mutex. If pre-check fails, returns false immediately (no blocking). Then takes osal_mutex_lock(_usbd_mutex, OSAL_TIMEOUT_WAIT_FOREVER) — on Pico this is mutex_enter_timeout_ms(mutex, UINT32_MAX). Inside mutex, re-checks and sets claimed=1. So tu_edpt_claim itself won't deadlock — it either returns false quickly (if busy/claimed) or acquires mutex and returns."
  implication: "The claim itself is NOT the deadlock point. The issue is upstream: what happens when claim fails repeatedly because the endpoint stays busy."

- timestamp: 2026-02-17T10:10:00Z
  checked: "write_flush() in midi_device.c (lines 216-238)"
  found: "write_flush() calls usbd_edpt_claim(rhport, midi->ep_in). If claim fails (endpoint busy from prior transfer), returns 0. If claim succeeds, reads from tx_ff into ep buffer, calls usbd_edpt_xfer which sets busy=1. Busy is only cleared by DCD_EVENT_XFER_COMPLETE processing in tud_task_ext() (line 702-703 of usbd.c)."
  implication: "CRITICAL: tud_task_ext() runs on Core0 (via loop's TinyUSBDevice.task() implicit call, or USB_D.read()). If Core0 is slow to process, endpoint stays busy, write_flush returns 0, tx_ff doesn't drain. This is the throttling mechanism."

- timestamp: 2026-02-17T10:15:00Z
  checked: "usbd_edpt_xfer() in usbd.c (lines 1413-1446)"
  found: "Sets ep_status[epnum][dir].busy = 1 BEFORE calling dcd_edpt_xfer. Busy cleared only in tud_task_ext when DCD_EVENT_XFER_COMPLETE arrives (line 702). If dcd_edpt_xfer fails, busy is cleared immediately (line 1440-1441)."
  implication: "Once a USB MIDI IN transfer is submitted from Core1 via write_flush, the endpoint stays busy until Core0 processes the completion. Core1 cannot submit another transfer until then."

- timestamp: 2026-02-17T10:20:00Z
  checked: "CDC write path from dualPrintf on Core1"
  found: "Serial.print(buffer) → Adafruit CDC → tud_cdc_write() → tu_edpt_stream_write() (tusb.c line 432). This writes to CDC tx_ff FIFO (256 bytes). When FIFO >= 64 bytes (MPS) or FIFO depth < MPS, calls tu_edpt_stream_write_xfer() which calls stream_claim() → usbd_edpt_claim() with _usbd_mutex. Same mutex as MIDI endpoint claim."
  implication: "Both MIDI and CDC TX share _usbd_mutex for endpoint claiming. Core1 calling both USB_D.sendX AND Serial.print hits the same mutex. But since mutex is non-recursive on Pico SDK, calling from same thread won't deadlock (claim/release is paired). Cross-core is safe because Pico mutex uses spinlock internally."

- timestamp: 2026-02-17T10:25:00Z
  checked: "tud_midi_n_stream_write while loop behavior under FIFO pressure"
  found: "The while loop (line 248) exits when tu_fifo_remaining(&midi->tx_ff) < 4. tx_ff is 64 bytes. If write_flush on line 355 fails to claim the endpoint (busy), no data drains from the FIFO. With the TE OP-XY sending MIDI at any rate, the FIFO fills in 16 packets. Once full, tud_midi_n_stream_write returns 0 for subsequent calls (while loop doesn't execute). This is NOT a blocking stall — the function returns. BUT: usb_host_wrapper.cpp callbacks call processMidiPacket() which calls usbh_onNoteOn → routeMidiMessage → forwardToInterface → USB_D.sendNoteOn. The MIDI Arduino library's sendNoteOn calls tud_midi_n_stream_write. If it returns 0, data is dropped but Core1 continues."
  implication: "The MIDI device write path alone should NOT cause Core1 to stall — it should just drop data when full. The stall must come from somewhere else."

- timestamp: 2026-02-17T10:30:00Z
  checked: "Serial.print (CDC write) blocking behavior from Core1"
  found: "tu_edpt_stream_write() (tusb.c line 432-453): When CDC FIFO has data, writes to ff via tu_fifo_write_n. If FIFO is full, tu_fifo_write_n returns 0 (non-overwritable FIFO). Then checks if FIFO >= MPS and calls tu_edpt_stream_write_xfer(). This path is also non-blocking. HOWEVER: The Arduino Serial.print() implementation may loop internally trying to write all bytes. Need to check if Adafruit's CDC write implementation blocks when buffer is full."
  implication: "If Arduino CDC Serial.print blocks waiting for space in CDC TX FIFO, and CDC TX FIFO only drains when Core0 processes USB device events, Core1 would block indefinitely when CDC buffer is full."

- timestamp: 2026-02-17T10:35:00Z
  checked: "Complete cross-core interaction analysis"
  found: "The fundamental problem: Core1 calls routeMidiMessage which calls BOTH forwardToInterface(USB_DEVICE) AND dualPrintf. Both generate USB Device TX traffic that can only complete when Core0 processes USB device events. If Core0 is momentarily busy (reading USB_D, processing web serial config, handling EEPROM), the USB device TX endpoints stay busy, FIFOs fill up, and Core1's writes either drop data or potentially block (in CDC case). Under sustained MIDI traffic, this creates backpressure that can stall Core1."
  implication: "The stall is most likely caused by the CDC Serial.print path blocking Core1 when the CDC TX FIFO is full and Core0 hasn't drained it. The MIDI path drops data but doesn't block. The CDC path might block."

- timestamp: 2026-02-18T12:20:00Z
  checked: "User test (CDC guard build)"
  found: "With if(Serial) guards in dualPrint/dualPrintln/dualPrintf, USB Host MIDI still does not trigger LED when serial monitor is closed"
  implication: "CDC Serial.print blocking is not the sole cause; another CDC-dependent gate or block exists"

- timestamp: 2026-02-18T12:22:00Z
  checked: "Serial usage and routing/LED code paths"
  found: "LED is triggered only in routeMidiMessage(); USB Host RX callback (tuh_midi_rx_cb) doesn't directly toggle LED. web_serial_config uses Serial.available() but only runs on core0 loop."
  implication: "If USB Host RX callback isn't firing when CDC is closed, LED won't blink even if routing code is intact"

- timestamp: 2026-02-18T12:25:00Z
  checked: "Diagnostic LED in USB Host RX callback"
  found: "User reports LED blinks when triggerUsbLED() is called in tuh_midi_rx_cb without serial monitor"
  implication: "USB Host RX callback is firing; failure is in routeMidiMessage or downstream output path"

- timestamp: 2026-02-18T12:28:00Z
  checked: "LED split test (RX vs routing)"
  found: "User reports BOTH Serial LED (RX callback) and USB LED (routeMidiMessage) blink on USB Host MIDI without serial monitor"
  implication: "Routing path appears to complete; need to confirm issue persists without diagnostic LED changes"

- timestamp: 2026-02-18T12:32:00Z
  checked: "User report on baseline behavior"
  found: "Issue is intermittent; sometimes USB LED blinks, sometimes not. User suspects host device not mounted or no MIDI received when LED doesn't blink"
  implication: "Need mount/umount visibility without Serial to determine whether failure is at host enumeration or at MIDI RX"

- timestamp: 2026-02-18T12:40:00Z
  checked: "Mount LED diagnostic"
  found: "User reports mount LED pulse is inconsistent; USB Host device does not always mount"
  implication: "Primary failure is intermittent enumeration/mount; focus on host power/PIO init/timing"

- timestamp: 2026-02-18T12:45:00Z
  checked: "Codebase VBUS handling search"
  found: "No VBUS enable/sense handling in firmware; host init only configures PIO and calls USBHost.begin"
  implication: "If host port lacks constant 5V VBUS, enumeration will be intermittent"

- timestamp: 2026-02-18T12:50:00Z
  checked: "User VBUS report"
  found: "VBUS is powered from computer USB and should be constant"
  implication: "Likely not a VBUS instability issue; investigate timing/reset on host init"

- timestamp: 2026-02-18T12:55:00Z
  checked: "USB PIO host init changes"
  found: "Host init now configures PIO USB via pio_usb_configuration_t (PIO_USB_DEFAULT_CONFIG, pin_dp=HOST_PIN_DP, Pico W swaps pio_rx_num=0/pio_tx_num=1) and calls USBHost.configure_pio_usb(1, &pio_cfg) before USBHost.begin(1)"
  implication: "Enumeration issues may be sensitive to PIO configuration and init order; verify timing and attach behavior"

- timestamp: 2026-02-18T13:05:00Z
  checked: "Mount visibility"
  found: "Identified intermittent mount as last confirmed issue; added blinking LED on device mount to observe mount events"
  implication: "Use mount LED to determine whether failures are enumeration/mount vs downstream MIDI routing"

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: "Cross-core USB Device stack access from Core1. When USB Host MIDI data arrives on Core1, the call chain is: tuh_midi_rx_cb → processMidiPacket → routeMidiMessage → forwardToInterface(USB_DEVICE) → USB_D.sendNoteOn() → tud_midi_n_stream_write() → write_flush() → usbd_edpt_claim() → tu_edpt_claim() → osal_mutex_lock(_usbd_mutex, OSAL_TIMEOUT_WAIT_FOREVER). This is mutex_enter_timeout_ms() with UINT32_MAX timeout. Simultaneously, dualPrintf() calls Serial.print() → CDC write → tu_edpt_stream_write() → tu_edpt_stream_write_xfer() → usbd_edpt_claim() → same _usbd_mutex. Core0 runs tud_task_ext() (USB device task) which processes DCD_EVENT_XFER_COMPLETE and clears busy/claimed flags, and also calls write_flush/xfer_cb which may claim endpoints. The _usbd_mutex is a Pico SDK mutex (not a spinlock) — it does NOT deadlock across cores by itself, but the endpoint busy flag can get stuck: if Core1 calls usbd_edpt_claim() which passes the pre-check (busy==0, claimed==0), takes the mutex, sets claimed=1, releases mutex, then calls usbd_edpt_xfer() which sets busy=1 and calls dcd_edpt_xfer() — but the DCD completion ISR fires on Core0 and queues a DCD_EVENT_XFER_COMPLETE, which is only processed by tud_task_ext() which is NOT being called from Core1. Core0 must call tud_task_ext() to clear busy/claimed. If Core0 is delayed (processing its own USB_D.read(), web serial config, etc.), the endpoint stays busy, and Core1's next write_flush() will fail usbd_edpt_claim() (busy!=0). This causes MIDI TX FIFO to fill up (only 64 bytes = 16 MIDI packets). Once tx_ff is full, tud_midi_n_stream_write() blocks in its while loop waiting for tu_fifo_remaining >= 4, which never frees because write_flush keeps failing because endpoint stays busy because Core0 hasn't processed the completion event yet. Meanwhile, the PIO USB SOF ISR fires every 1ms on the core that created the alarm pool (Core1 if host was init'd there), which runs USB transactions inside ISR context. If Core1 is spinning in tud_midi_n_stream_write's while loop, the SOF ISR can still fire, but if there's any spinlock contention between the SOF ISR and the mutex Core1 is holding, OR if the SOF ISR needs to complete before the FIFO drains, Core1 effectively deadlocks."
fix: "Added `if (Serial)` guards before all Serial.print/Serial.println calls in serial_utils.cpp (dualPrint, dualPrintln, dualPrintf). When no CDC serial monitor is connected, `if (Serial)` evaluates to false (Adafruit_USBD_CDC::operator bool() returns DTR state), so the blocking write is skipped entirely. Serial2 (UART) always writes unconditionally. Build succeeds."
verification: "Awaiting hardware test"
verification: "Test failed: USB Host MIDI still does not trigger LED when serial monitor is closed"
files_changed: ["rp2040/serial_utils.cpp"]
