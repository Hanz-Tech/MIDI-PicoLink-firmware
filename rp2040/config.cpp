#include "config.h"
#include "midi_filters.h"
#include <EEPROM.h>

 // EEPROM layout: 3*8 + 16 = 40 bytes (bools as bytes)
#define CONFIG_EEPROM_SIZE 40
#define EEPROM_START_ADDR 0

void saveConfigToEEPROM() {
    EEPROM.begin(CONFIG_EEPROM_SIZE);
    int addr = EEPROM_START_ADDR;
    // Save midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            EEPROM.write(addr++, getMidiFilterState(iface, msg) ? 1 : 0);
        }
    }
    // Save enabledChannels
    for (int ch = 0; ch < 16; ++ch) {
        EEPROM.write(addr++, getChannelEnabledState(ch) ? 1 : 0);
    }
    EEPROM.commit();
    EEPROM.end();
}

void loadConfigFromEEPROM() {
    EEPROM.begin(CONFIG_EEPROM_SIZE);
    int addr = EEPROM_START_ADDR;
    // Load midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            setMidiFilterState(iface, msg, EEPROM.read(addr++) ? true : false);
        }
    }
    // Load enabledChannels
    for (int ch = 0; ch < 16; ++ch) {
        setChannelEnabledState(ch, EEPROM.read(addr++) ? true : false);
    }
    EEPROM.end();
}

void configToJson(JsonDocument& doc) {
    // Filters
    JsonArray filters = doc["filters"].to<JsonArray>();
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        JsonArray ifaceArr = filters.add<JsonArray>();
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            ifaceArr.add(getMidiFilterState(iface, msg));
        }
    }
    // Channels
    JsonArray channels = doc["channels"].to<JsonArray>();
    for (int ch = 0; ch < 16; ++ch) {
        channels.add(getChannelEnabledState(ch));
    }
}

bool updateConfigFromJson(const JsonDocument& doc) {
    // Filters
    JsonArray filtersArr = ((JsonDocument&)doc)["filters"].as<JsonArray>();
    if (filtersArr.isNull()) {
        Serial.println("[DEBUG] updateConfigFromJson: 'filters' is not an array or missing (as<JsonArray>() isNull).");
        return false;
    }
    if (filtersArr.size() != MIDI_INTERFACE_COUNT) {
        Serial.print("[DEBUG] updateConfigFromJson: 'filters' array size is ");
        Serial.print(filtersArr.size());
        Serial.print(", expected ");
        Serial.println(MIDI_INTERFACE_COUNT);
        return false;
    }

    // Process filters array with indexed loop
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; iface++) {
        JsonArray ifaceArr = filtersArr[iface].as<JsonArray>();
        if (ifaceArr.isNull()) {
            Serial.print("[DEBUG] updateConfigFromJson: filters[");
            Serial.print(iface);
            Serial.println("] is not an array (as<JsonArray>() isNull).");
            return false;
        }
        if (ifaceArr.size() != MIDI_MSG_COUNT) {
            Serial.print("[DEBUG] updateConfigFromJson: filters[");
            Serial.print(iface);
            Serial.print("] size is ");
            Serial.print(ifaceArr.size());
            Serial.print(", expected ");
            Serial.println(MIDI_MSG_COUNT);
            return false;
        }
        // Process inner array with indexed loop
        for (int msg = 0; msg < MIDI_MSG_COUNT; msg++) {
            setMidiFilterState(iface, msg, ifaceArr[msg].as<bool>());
        }
    }

    // Channels
    JsonArray channelsArr = ((JsonDocument&)doc)["channels"].as<JsonArray>();
    if (channelsArr.isNull()) {
        Serial.println("[DEBUG] updateConfigFromJson: 'channels' is not an array or missing (as<JsonArray>() isNull).");
        return false;
    }
    if (channelsArr.size() != 16) {
        Serial.print("[DEBUG] updateConfigFromJson: 'channels' array size is ");
        Serial.print(channelsArr.size());
        Serial.println(", expected 16");
        return false;
    }

    // Process channels array with indexed loop
    for (int ch = 0; ch < 16; ch++) {
        setChannelEnabledState(ch, channelsArr[ch].as<bool>());
    }

    Serial.println("[DEBUG] updateConfigFromJson: config accepted.");
    return true;
}
