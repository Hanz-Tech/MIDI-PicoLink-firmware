#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "midi_filters.h"

// EEPROM size in bytes
#define EEPROM_SIZE 1024

// EEPROM configuration version
#define CONFIG_VERSION 1

// Structure to hold MIDI filter configuration
struct MidiFilterConfig {
    bool filters[MIDI_INTERFACE_COUNT][MIDI_MSG_COUNT];
};

// Initialize the EEPROM
void setupEEPROM();

// Load filter settings from EEPROM
bool loadFilterSettings();

// Save current filter settings to EEPROM
bool saveFilterSettings();

// Process JSON commands received via Serial
void processSerialCommands();

// Export current filter settings as JSON
String exportFilterSettingsJson();

// Import filter settings from JSON
bool importFilterSettingsJson(const String& jsonString);

#endif // EEPROM_CONFIG_H
