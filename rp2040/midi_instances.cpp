#include "midi_instances.h"

// Define the USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create the MIDI interface instance


MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USB_D);