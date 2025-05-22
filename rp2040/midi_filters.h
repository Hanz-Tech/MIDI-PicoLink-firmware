#ifndef MIDI_FILTERS_H
#define MIDI_FILTERS_H

#include <Arduino.h>

// Enum for different MIDI message types that can be filtered
typedef enum {
    MIDI_MSG_NOTE = 0,              // Controls both Note On and Note Off
    MIDI_MSG_POLY_AFTERTOUCH,
    MIDI_MSG_CONTROL_CHANGE,
    MIDI_MSG_PROGRAM_CHANGE,
    MIDI_MSG_CHANNEL_AFTERTOUCH,
    MIDI_MSG_PITCH_BEND,
    MIDI_MSG_SYSEX,
    MIDI_MSG_REALTIME,
    MIDI_MSG_COUNT // Used to determine the size of the filter array (now 8)
} MidiMsgType;

// Enum for different MIDI interfaces
typedef enum {
    MIDI_INTERFACE_SERIAL = 0,
    MIDI_INTERFACE_USB_DEVICE,
    MIDI_INTERFACE_USB_HOST,
    MIDI_INTERFACE_COUNT // Used to determine the size of the filter array
} MidiInterfaceType;

// Initialize the MIDI filters
void setupMidiFilters();

// Functions to enable/disable filters
void setMidiFilter(MidiInterfaceType interface, MidiMsgType msgType, bool enabled);
void enableMidiFilter(MidiInterfaceType interface, MidiMsgType msgType);
void disableMidiFilter(MidiInterfaceType interface, MidiMsgType msgType);

// Function to check if a message type is filtered for a specific interface
bool isMidiFiltered(MidiInterfaceType interface, MidiMsgType msgType);

// Helper functions to enable/disable all filters for a specific interface
void enableAllFilters(MidiInterfaceType interface);
void disableAllFilters(MidiInterfaceType interface);

// Helper functions to enable/disable a specific message type for all interfaces
void filterMessageTypeForAll(MidiMsgType msgType, bool enabled);

void disableNoteForAll();
void disablePolyAftertouchForAll();
void disableControlChangeForAll();
void disableProgramChangeForAll();
void disableChannelAftertouchForAll();
void disablePitchBendForAll();
void disableSysExForAll();
void disableRealtimeForAll();


// --- Global MIDI Channel Filter ---

// Returns true if the given MIDI channel (1-16) is enabled
bool isChannelEnabled(byte channel);
// Enable/disable a specific channel (1-16)
void setChannelEnabled(byte channel, bool enabled);
// Enable all channels
void enableAllChannels();
// Disable all channels
void disableAllChannels();

// --- Config Storage Helpers ---

// Get/set filter state directly (used by config.cpp)
bool getMidiFilterState(int interface, int msgType);
void setMidiFilterState(int interface, int msgType, bool state);
bool getChannelEnabledState(int channel); // 0-15
void setChannelEnabledState(int channel, bool state); // 0-15

#endif // MIDI_FILTERS_H
