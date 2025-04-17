#ifndef SERIAL_MIDI_HANDLER_H
#define SERIAL_MIDI_HANDLER_H

#include <Arduino.h>
#include <MIDI.h> // Include MIDI library header

// Declare functions for setup and loop processing for Serial MIDI
void setupSerialMidi();
void loopSerialMidi();

// Declare functions to *send* MIDI messages *to* the Serial MIDI port
// These will be called from the USB Host and USB Device handlers in the main sketch
void sendSerialMidiNoteOn(byte channel, byte note, byte velocity);
void sendSerialMidiNoteOff(byte channel, byte note, byte velocity);
void sendSerialMidiAfterTouch(byte channel, byte note, byte amount); // For Polyphonic AT
void sendSerialMidiControlChange(byte channel, byte controller, byte value);
void sendSerialMidiProgramChange(byte channel, byte program);
void sendSerialMidiAfterTouchChannel(byte channel, byte pressure); // For Channel AT
void sendSerialMidiPitchBend(byte channel, int bend);
void sendSerialMidiSysEx(unsigned size, const byte *array);
void sendSerialMidiRealTime(midi::MidiType type);

#endif // SERIAL_MIDI_HANDLER_H