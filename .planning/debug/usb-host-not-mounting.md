---
status: investigating
trigger: "Investigate issue: usb-host-not-mounting"
created: 2026-02-18T07:35:15Z
updated: 2026-02-18T07:45:31Z
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: BeatStep Pro may enumerate but not send MIDI data; GitHub issue #14 is about no MIDI data despite successful enumeration, which would be related only if your device actually mounts.
test: compare your symptoms with issue #14 (enumeration logs vs no mount).
expecting: if your logs show MIDI descriptor parsed + endpoints opened, then issue #14 is likely related; if your device never mounts, it's a different problem.
next_action: ask user for mount/enumeration logs or confirmation whether it enumerates at all.

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: USB host device is mounted
actual: The usb device doesn't seem to be recognized and is not mounted
errors: No
reproduction: Plug in the usb host device
started: I don't think it ever worked

## Eliminated
<!-- APPEND only - prevents re-investigating -->

## Evidence
<!-- APPEND only - facts discovered -->

- timestamp: 2026-02-18T07:36:49Z
  checked: repo file search for pio_usb_configuration.h
  found: file not present in repo via glob; only include found in rp2040.ino
  implication: header likely comes from external library/build or is missing, affecting PIO USB host defaults

- timestamp: 2026-02-18T07:35:34Z
  checked: rp2040/usb_host_wrapper.cpp
  found: USB host task calls USBHost.task() and uses TinyUSB host callbacks for mount/umount/midi_rx.
  implication: If USBHost.task() isn't run or host init fails, devices won't mount.

- timestamp: 2026-02-18T07:35:34Z
  checked: rp2040/rp2040.ino
  found: USBHost configured in setup1() with PIO USB (pin_dp=12), USBHost.configure_pio_usb(1, &pio_cfg), USBHost.begin(1), loop1() calls usb_host_wrapper_task().
  implication: Host init and polling depend on core1 setup/loop1 running; PIO USB host uses DP pin 12 and fixed config.

- timestamp: 2026-02-18T07:38:21Z
  checked: user checkpoint response
  found: pio_usb_configuration.h lives in external Pico_PIO_USB library; PIO_USB_DEFAULT_CONFIG uses PIO_USB_DP_PIN_DEFAULT (default 0 unless overridden)
  implication: project may be using default DP pin 0 unless overriding, which could conflict with pin 12 usage and prevent host enumeration

- timestamp: 2026-02-18T07:39:28Z
  checked: repo grep for PIO_USB pin/default macro overrides
  found: no overrides of PIO_USB_DP_PIN_DEFAULT or PIO_USB_DEFAULT_CONFIG in repo (only references in rp2040.ino and debug files)
  implication: host pin config relies solely on runtime pio_cfg.pin_dp assignment in rp2040.ino

- timestamp: 2026-02-18T07:40:36Z
  checked: external Pico_PIO_USB pio_usb_configuration.h
  found: PIO_USB_DEFAULT_CONFIG sets pin_dp to PIO_USB_DP_PIN_DEFAULT (default 0), pinout defaults to PIO_USB_PINOUT_DPDM
  implication: runtime config must override pin_dp/pinout if hardware differs; defaults are DP=0 and DPDM

- timestamp: 2026-02-18T07:41:21Z
  checked: repo grep for pinout/DP/DM wiring references
  found: only HOST_PIN_DP comment in rp2040.ino states D- = D+ + 1; no pinout overrides in code
  implication: code assumes DPDM pinout; if hardware wiring differs, host won't enumerate

- timestamp: 2026-02-18T07:44:25Z
  checked: repo documentation for USB host pin wiring
  found: architecture/integrations docs state USB Host D+ = GPIO 12, D- = GPIO 13; no alternative pinout or DMDP wiring documented
  implication: firmware pin assumption matches documented hardware pinout (DPDM on 12/13)

- timestamp: 2026-02-18T08:10:12Z
  checked: user checkpoint response
  found: user reports some USB devices work and some don’t; user believes issue is not hardware; no wiring confirmation provided
  implication: problem may be device-class/power related or stack configuration rather than total hardware failure

- timestamp: 2026-02-18T08:14:58Z
  checked: repo for TinyUSB host config (tusb_config.h or CFG_TUH settings)
  found: no tusb_config.h or TinyUSB host config files in repo; only MIDI host callbacks are defined
  implication: USB host class support likely comes from external library defaults; project appears to handle MIDI class only, so non-MIDI devices may not mount as “working.”

- timestamp: 2026-02-18T08:20:40Z
  checked: Adafruit TinyUSB rp2040 host config (external)
  found: host config enables MSC/HID/CDC by default; no project overrides found
  implication: host stack supports multiple classes; failure likely not due to class config in repo

- timestamp: 2026-02-18T08:22:10Z
  checked: user report update
  found: some USB devices mount and some do not; user believes hardware is correct; host device powers on but does not mount
  implication: partial functionality suggests power/class/compatibility differences rather than total host failure

- timestamp: 2026-02-18T08:26:04Z
  checked: GitHub issue rppicomidi/usb_midi_host#14
  found: BeatStep Pro is recognized with two MIDI cables; descriptors parsed; no MIDI data received
  implication: issue #14 is about data flow after successful enumeration, not about mounting failure

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: ""
fix: ""
verification: ""
files_changed: []
