#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>

// Save/load filter and channel config to/from EEPROM
void saveConfigToEEPROM();
void loadConfigFromEEPROM();

// Serialize/deserialize filter and channel config to/from JSON
void configToJson(JsonDocument& doc);
bool updateConfigFromJson(const JsonDocument& doc);

#endif // CONFIG_H


/*
{
  "command": "SAVEALL",
  "filters": [
    // SERIAL interface filters (8 booleans)
    [false, false, false, false, false, false, false, false],
    // USB DEVICE interface filters (8 booleans)
    [true, false, true, false, false, false, true, false],
    // USB HOST interface filters (8 booleans)
    [false, true, false, true, false, true, false, true]
  ],
  "destFilters": [
    // SERIAL destination filters (8 booleans)
    [false, false, false, false, false, false, false, false],
    // USB DEVICE destination filters (8 booleans)
    [true, false, true, false, false, false, true, false],
    // USB HOST destination filters (8 booleans)
    [false, true, false, true, false, true, false, true]
  ],
  "channels": [true, true, true, true, false, false, true, true, true, true, true, true, true, true, true, true]
}

### First Dimension (Interfaces):

The array has 3 elements representing the three MIDI interfaces in the system:

1. `filters[0][...]` - MIDI_INTERFACE_SERIAL: Controls filtering for the hardware Serial MIDI port
2. `filters[1][...]` - MIDI_INTERFACE_USB_DEVICE: Controls filtering for the USB MIDI Device port (when connected to a computer)
3. `filters[2][...]` - MIDI_INTERFACE_USB_HOST: Controls filtering for the USB MIDI Host port (when connected to MIDI devices)

### Second Dimension (Message Types):

Each interface has 9 boolean values representing different MIDI message types:

1. `filters[...][0]` - MIDI_MSG_NOTE_ON/OFF: Note On/OFF messages (when a key is pressed)
3. `filters[...][1]` - MIDI_MSG_POLY_AFTERTOUCH: Polyphonic Aftertouch messages (per-note pressure)
4. `filters[...][2]` - MIDI_MSG_CONTROL_CHANGE: Control Change messages (knobs, sliders, pedals)
5. `filters[...][3]` - MIDI_MSG_PROGRAM_CHANGE: Program Change messages (patch/preset changes)
6. `filters[...][4]` - MIDI_MSG_CHANNEL_AFTERTOUCH: Channel Aftertouch messages (channel pressure)
7. `filters[...][5]` - MIDI_MSG_PITCH_BEND: Pitch Bend messages
8. `filters[...][6]` - MIDI_MSG_SYSEX: System Exclusive messages (device-specific data)
9. `filters[...][7]` - MIDI_MSG_REALTIME: MIDI Realtime messages (clock, start, stop, continue)


*/

