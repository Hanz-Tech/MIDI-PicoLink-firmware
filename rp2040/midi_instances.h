#ifndef MIDI_INSTANCES_H
#define MIDI_INSTANCES_H

#include <Arduino.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>

// Declare the USB MIDI object
extern Adafruit_USBD_MIDI usb_midi;

// Declare the MIDI interface with the CORRECT type
extern midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> USB_D;

#endif // MIDI_INSTANCES_H