#ifndef MIDI_ROUTER_H
#define MIDI_ROUTER_H

#include <Arduino.h>
#include <MIDI.h>
#include "midi_filters.h"

// Source interface that received the MIDI message
// (reuses MidiInterfaceType values but semantically means "where it came from")
typedef enum {
    MIDI_SOURCE_SERIAL = MIDI_INTERFACE_SERIAL,
    MIDI_SOURCE_USB_DEVICE = MIDI_INTERFACE_USB_DEVICE,
    MIDI_SOURCE_USB_HOST = MIDI_INTERFACE_USB_HOST,
    MIDI_SOURCE_INTERNAL = MIDI_INTERFACE_COUNT
} MidiSource;

// Destination routing masks
static const byte ROUTE_TO_SERIAL = (1 << 0);
static const byte ROUTE_TO_USB_DEVICE = (1 << 1);
static const byte ROUTE_TO_USB_HOST = (1 << 2);
static const byte ROUTE_TO_ALL = (ROUTE_TO_SERIAL | ROUTE_TO_USB_DEVICE | ROUTE_TO_USB_HOST);

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
void routeMidiMessage(MidiSource source, const MidiMessage &msg, byte destMask);

inline void routeMidiMessage(MidiInterfaceType source, const MidiMessage &msg) {
    routeMidiMessage(static_cast<MidiSource>(source), msg);
}

#endif // MIDI_ROUTER_H
