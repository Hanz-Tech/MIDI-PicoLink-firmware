#ifndef MIDI_ROUTER_H
#define MIDI_ROUTER_H

#include <Arduino.h>
#include <MIDI.h>
#include "midi_filters.h"

// Source interface that received the MIDI message
// (reuses MidiInterfaceType values but semantically means "where it came from")
typedef MidiInterfaceType MidiSource;

// Union-style struct to hold any MIDI message data
typedef struct {
    MidiMsgType type;       // Message type (NOTE, CC, etc.)
    byte subType;            // 0 = Note On/default, 1 = Note Off (NOTE type)
    byte channel;           // MIDI channel 1-16 (0 for system messages)
    byte data1;             // Note number, CC number, program number, etc.
    byte data2;             // Velocity, CC value, aftertouch amount, etc.
    int pitchBend;          // Pitch bend value (only for PITCH_BEND type)
    byte *sysexData;        // Pointer to SysEx data (only for SYSEX type)
    unsigned sysexSize;     // SysEx data size (only for SYSEX type)
    midi::MidiType rtType;  // Real-time message subtype (Clock, Start, Continue, Stop)
} MidiMessage;

// Route a MIDI message from source to all other interfaces, applying filters
void routeMidiMessage(MidiSource source, const MidiMessage &msg);

#endif // MIDI_ROUTER_H
