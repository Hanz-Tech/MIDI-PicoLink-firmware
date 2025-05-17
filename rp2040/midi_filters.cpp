#include "midi_filters.h"

// Global filter array: [interface][message type]
// true = message is filtered (blocked), false = message passes through
// Removed 'static' to make it accessible from eeprom_config.cpp
bool midiFilters[MIDI_INTERFACE_COUNT][MIDI_MSG_COUNT] = {0};

void setupMidiFilters() {
    // Initialize all filters to false (no filtering)
    for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = false;
        }
    }
    
    Serial.println("MIDI Filters: Initialized (all messages passing through)");
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
        
        Serial.printf("MIDI Filter: %s messages on %s interface %s\n", 
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
        Serial.printf("MIDI Filter: ALL messages on %s interface BLOCKED\n", 
            interfaceNames[interface]);
    }
}

void disableAllFilters(MidiInterfaceType interface) {
    if (interface < MIDI_INTERFACE_COUNT) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = false;
        }
        
        const char* interfaceNames[] = {"Serial", "USB Device", "USB Host"};
        Serial.printf("MIDI Filter: ALL messages on %s interface ENABLED\n", 
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
        
        Serial.printf("MIDI Filter: %s messages on ALL interfaces %s\n", 
            msgTypeNames[msgType], 
            enabled ? "BLOCKED" : "ENABLED");
    }
}
