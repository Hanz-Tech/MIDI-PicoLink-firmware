# USB Host Not Mounting — Findings Summary

## Context
- **Issue:** USB host device does not mount when plugged in.
- **Expected:** USB host device mounts.
- **Actual:** Device not recognized / not mounted.
- **Errors:** None reported.
- **Reproduction:** Plug in USB host device.
- **Timeline:** User reports it likely never worked.

## Evidence Collected
- Firmware host init occurs in `rp2040.ino` `setup1()`:
  - `USBHost.configure_pio_usb(1, &pio_cfg)` then `USBHost.begin(1)`
  - `HOST_PIN_DP` set to **12** and `loop1()` calls `usb_host_wrapper_task()`
- External library `Pico_PIO_USB` is used for PIO USB host configuration.
- `pio_usb_configuration.h` is **external**, located at:
  - `/home/han/Arduino/libraries/Pico_PIO_USB/src/pio_usb_configuration.h`
  - `PIO_USB_DEFAULT_CONFIG` uses `PIO_USB_DP_PIN_DEFAULT` (defaults to **0** unless overridden)
  - Firmware overrides `pin_dp` at runtime to **HOST_PIN_DP** (12)
- Project docs state USB Host wiring is **D+ = GPIO12**, **D- = GPIO13** (assumes DPDM).
- User reports **some USB devices work, some don’t**, suggesting power/compatibility differences.

## Current Hypotheses
1. **VBUS power missing or unstable**
   - Bus‑powered devices won’t enumerate without 5V VBUS.
   - Self‑powered devices could still work.
2. **USB D+/D- pin wiring mismatch**
   - If D+/D- are swapped or on different GPIOs, enumeration may be intermittent.
   - User has not confirmed physical wiring yet.

## Checkpoints Requested
1. **Physical D+/D- wiring confirmation**
   - Need exact GPIO mapping for USB D+ and D-.

2. **VBUS (5V) presence verification**
   - Measure VBUS at host port (pin 1) to GND, with/without device.
   - Identify if device is bus‑powered or self‑powered.
   - Confirm any VBUS enable/switch wiring.

## Next Step (Pending User Input)
- Confirm **VBUS voltage** and **D+/D- wiring**.
- This will determine whether the failure is **power-related** or **pin configuration-related**.
