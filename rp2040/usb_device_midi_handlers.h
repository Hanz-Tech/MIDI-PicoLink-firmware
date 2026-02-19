#ifndef USB_DEVICE_MIDI_HANDLERS_H
#define USB_DEVICE_MIDI_HANDLERS_H

#include <Arduino.h>

// Register USB Device MIDI callback handlers on the USB_D instance
// Call this from setup() after USB_D.begin()
void setupUsbDeviceHandlers();

#endif // USB_DEVICE_MIDI_HANDLERS_H
