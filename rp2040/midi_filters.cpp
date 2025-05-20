#include "midi_filters.h"
#include "serial_utils.h"

// Global filter array: [interface][message type]
// true = message is filtered (blocked), false = message passes through
static bool midiFilters[MIDI_INTERFACE_COUNT][MIDI_MSG_COUNT] = {0};

void setupMidiFilters() {
    // Initialize all filters to false (no filtering)
    for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = false;
        }
    }
    
    dualPrintln("MIDI Filters: Initialized (all messages passing through)");
}

void setMidiFilter(MidiInterfaceType interface, MidiMsgType msgType, bool enabled) {
    if (interface < MIDI_INTERFACE_COUNT && msgType < MIDI_MSG_COUNT) {
        midiFilters[interface][msgType] = enabled;
        
        // Log the filter change
        const char* interfaceNames[] = {"Serial", "USB Device", "USB Host"};
        const char* msgTypeNames[] = {
            "Note On", "Note Off", "Poly Aftertouch", "Control Change", 
            "Program Change", "Channel Aftertouch", "Pitch Bend", 
            "SysEx", "Realtime"
        };
        
        dualPrintf("MIDI Filter: %s messages on %s interface %s\n", 
            msgTypeNames[msgType], 
            interfaceNames[interface], 
            enabled ? "BLOCKED" : "ENABLED");
    }
}

void enableMidiFilter(MidiInterfaceType interface, MidiMsgType msgType) {
    setMidiFilter(interface, msgType, true);
}

void disableMidiFilter(MidiInterfaceType interface, MidiMsgType msgType) {
    setMidiFilter(interface, msgType, false);
}

bool isMidiFiltered(MidiInterfaceType interface, MidiMsgType msgType) {
    if (interface < MIDI_INTERFACE_COUNT && msgType < MIDI_MSG_COUNT) {
        return midiFilters[interface][msgType];
    }
    return false; // Default to not filtered if invalid parameters
}

void enableAllFilters(MidiInterfaceType interface) {
    if (interface < MIDI_INTERFACE_COUNT) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = true;
        }
        
        const char* interfaceNames[] = {"Serial", "USB Device", "USB Host"};
        dualPrintf("MIDI Filter: ALL messages on %s interface BLOCKED\n", 
            interfaceNames[interface]);
    }
}

void disableAllFilters(MidiInterfaceType interface) {
    if (interface < MIDI_INTERFACE_COUNT) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = false;
        }
        
        const char* interfaceNames[] = {"Serial", "USB Device", "USB Host"};
        dualPrintf("MIDI Filter: ALL messages on %s interface ENABLED\n", 
            interfaceNames[interface]);
    }
}

void filterMessageTypeForAll(MidiMsgType msgType, bool enabled) {
    if (msgType < MIDI_MSG_COUNT) {
        for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
            midiFilters[interface][msgType] = enabled;
        }
        
        const char* msgTypeNames[] = {
            "Note On", "Note Off", "Poly Aftertouch", "Control Change", 
            "Program Change", "Channel Aftertouch", "Pitch Bend", 
            "SysEx", "Realtime"
        };
        
        dualPrintf("MIDI Filter: %s messages on ALL interfaces %s\n", 
            msgTypeNames[msgType], 
            enabled ? "BLOCKED" : "ENABLED");
    }
}

// Functions to disable specific message types for all interfaces
void disableNoteForAll() {
    filterMessageTypeForAll(MIDI_MSG_NOTE_ON, true);
    filterMessageTypeForAll(MIDI_MSG_NOTE_OFF, true);

}

void disablePolyAftertouchForAll() {
    filterMessageTypeForAll(MIDI_MSG_POLY_AFTERTOUCH, true);
}

void disableControlChangeForAll() {
    filterMessageTypeForAll(MIDI_MSG_CONTROL_CHANGE, true);
}

void disableProgramChangeForAll() {
    filterMessageTypeForAll(MIDI_MSG_PROGRAM_CHANGE, true);
}

void disableChannelAftertouchForAll() {
    filterMessageTypeForAll(MIDI_MSG_CHANNEL_AFTERTOUCH, true);
}

void disablePitchBendForAll() {
    filterMessageTypeForAll(MIDI_MSG_PITCH_BEND, true);
}

void disableSysExForAll() {
    filterMessageTypeForAll(MIDI_MSG_SYSEX, true);
}

void disableRealtimeForAll() {
    filterMessageTypeForAll(MIDI_MSG_REALTIME, true);
}

// --- Global MIDI Channel Filter Implementation ---
static bool enabledChannels[16] = {
    true, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true
};

bool isChannelEnabled(byte channel) {
    // MIDI channels are 1-16
    if (channel < 1 || channel > 16) return false;
    return enabledChannels[channel - 1];
}

void setChannelEnabled(byte channel, bool enabled) {
    if (channel >= 1 && channel <= 16) {
        enabledChannels[channel - 1] = enabled;
        dualPrintf("MIDI Channel Filter: Channel %d %s\n", channel, enabled ? "ENABLED" : "DISABLED");
    }
}

void enableAllChannels() {
    for (int i = 0; i < 16; ++i) enabledChannels[i] = true;
    dualPrintln("MIDI Channel Filter: ALL channels ENABLED");
}

void disableAllChannels() {
    for (int i = 0; i < 16; ++i) enabledChannels[i] = false;
    dualPrintln("MIDI Channel Filter: ALL channels DISABLED");
}
