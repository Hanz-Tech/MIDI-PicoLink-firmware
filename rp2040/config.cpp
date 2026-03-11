#include "config.h"
#include "midi_filters.h"
#include "imu_handler.h"
#include "serial_utils.h"
#include <EEPROM.h>

 // EEPROM layout: 
 // - Source MIDI filters: 3*8 = 24 bytes (bools as bytes)
 // - Destination MIDI filters: 3*8 = 24 bytes (bools as bytes)
 // - Channels: 16 bytes (bools as bytes)  
 // - IMU config: 27 bytes
 // Total: 91 bytes
#define CONFIG_EEPROM_SIZE 128
#define EEPROM_START_ADDR 0

void saveConfigToEEPROM() {
    EEPROM.begin(CONFIG_EEPROM_SIZE);
    int addr = EEPROM_START_ADDR;
    
    // Save source midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            EEPROM.write(addr++, getMidiFilterState(iface, msg) ? 1 : 0);
        }
    }

    // Save destination midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            EEPROM.write(addr++, getMidiDestFilterState(iface, msg) ? 1 : 0);
        }
    }
    
    // Save enabledChannels
    for (int ch = 0; ch < 16; ++ch) {
        EEPROM.write(addr++, getChannelEnabledState(ch) ? 1 : 0);
    }
    
    // Save IMU configuration
    IMUConfig imuConfig = getIMUConfig();
    // Note: No global enabled flag stored - calculated automatically from individual axis enables
    
    dualPrintf("[DEBUG] Saving IMU config: Roll en=%d, ch=%d, cc=%d\n", 
               imuConfig.rollEnabled, imuConfig.rollMidiChannel, imuConfig.rollMidiCC);
    dualPrintf("[DEBUG] Pitch en=%d, ch=%d, cc=%d\n", 
               imuConfig.pitchEnabled, imuConfig.pitchMidiChannel, imuConfig.pitchMidiCC);
    dualPrintf("[DEBUG] Yaw en=%d, ch=%d, cc=%d\n", 
               imuConfig.yawEnabled, imuConfig.yawMidiChannel, imuConfig.yawMidiCC);
    
    // Roll configuration
    EEPROM.write(addr++, imuConfig.rollEnabled ? 1 : 0);
    EEPROM.write(addr++, imuConfig.rollMidiChannel);
    EEPROM.write(addr++, imuConfig.rollMidiCC);
    EEPROM.write(addr++, imuConfig.rollDefaultValue);
    EEPROM.write(addr++, imuConfig.rollToSerial ? 1 : 0);
    EEPROM.write(addr++, imuConfig.rollToUSBDevice ? 1 : 0);
    EEPROM.write(addr++, imuConfig.rollToUSBHost ? 1 : 0);
    // Store sensitivity and range as integers (multiply by 10)
    EEPROM.write(addr++, (uint8_t)(imuConfig.rollSensitivity * 10));
    EEPROM.write(addr++, (uint8_t)(imuConfig.rollRange));
    
    // Pitch configuration
    EEPROM.write(addr++, imuConfig.pitchEnabled ? 1 : 0);
    EEPROM.write(addr++, imuConfig.pitchMidiChannel);
    EEPROM.write(addr++, imuConfig.pitchMidiCC);
    EEPROM.write(addr++, imuConfig.pitchDefaultValue);
    EEPROM.write(addr++, imuConfig.pitchToSerial ? 1 : 0);
    EEPROM.write(addr++, imuConfig.pitchToUSBDevice ? 1 : 0);
    EEPROM.write(addr++, imuConfig.pitchToUSBHost ? 1 : 0);
    EEPROM.write(addr++, (uint8_t)(imuConfig.pitchSensitivity * 10));
    EEPROM.write(addr++, (uint8_t)(imuConfig.pitchRange));
    
    // Yaw configuration
    EEPROM.write(addr++, imuConfig.yawEnabled ? 1 : 0);
    EEPROM.write(addr++, imuConfig.yawMidiChannel);
    EEPROM.write(addr++, imuConfig.yawMidiCC);
    EEPROM.write(addr++, imuConfig.yawDefaultValue);
    EEPROM.write(addr++, imuConfig.yawToSerial ? 1 : 0);
    EEPROM.write(addr++, imuConfig.yawToUSBDevice ? 1 : 0);
    EEPROM.write(addr++, imuConfig.yawToUSBHost ? 1 : 0);
    EEPROM.write(addr++, (uint8_t)(imuConfig.yawSensitivity * 10));
    EEPROM.write(addr++, (uint8_t)(imuConfig.yawRange));
    
    dualPrintf("[DEBUG] IMU config saved to EEPROM (used %d bytes total)\n", addr);
    
    EEPROM.commit();
    EEPROM.end();
}

void loadConfigFromEEPROM() {
    EEPROM.begin(CONFIG_EEPROM_SIZE);
    int addr = EEPROM_START_ADDR;
    
    // Load source midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            setMidiFilterState(iface, msg, EEPROM.read(addr++) ? true : false);
        }
    }

    // Load destination midiFilters
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            setMidiDestFilterState(iface, msg, EEPROM.read(addr++) ? true : false);
        }
    }
    
    // Load enabledChannels
    for (int ch = 0; ch < 16; ++ch) {
        setChannelEnabledState(ch, EEPROM.read(addr++) ? true : false);
    }
    
    // Load IMU configuration if enough data exists
    // IMU config needs 27 bytes: 9*3 = 27 bytes total (removed global enable)
    dualPrintf("[DEBUG] EEPROM load: addr=%d, need 27 bytes, size=%d\n", addr, CONFIG_EEPROM_SIZE);
    if (addr + 27 <= CONFIG_EEPROM_SIZE) { // Check if we have enough bytes for IMU config
        dualPrintln("[DEBUG] Loading IMU config from EEPROM...");
        IMUConfig imuConfig;
        // Note: No global enabled flag loaded - calculated automatically from individual axis enables
        
        // Roll configuration
        imuConfig.rollEnabled = EEPROM.read(addr++) ? true : false;
        imuConfig.rollMidiChannel = EEPROM.read(addr++);
        imuConfig.rollMidiCC = EEPROM.read(addr++);
        imuConfig.rollDefaultValue = EEPROM.read(addr++);
        imuConfig.rollToSerial = EEPROM.read(addr++) ? true : false;
        imuConfig.rollToUSBDevice = EEPROM.read(addr++) ? true : false;
        imuConfig.rollToUSBHost = EEPROM.read(addr++) ? true : false;
        imuConfig.rollSensitivity = EEPROM.read(addr++) / 10.0f;
        imuConfig.rollRange = EEPROM.read(addr++);
        
        dualPrintf("[DEBUG] Roll loaded: en=%d, ch=%d, cc=%d, def=%d, sens=%.1f, range=%.0f\n", 
                   imuConfig.rollEnabled, imuConfig.rollMidiChannel, imuConfig.rollMidiCC, 
                   imuConfig.rollDefaultValue, imuConfig.rollSensitivity, imuConfig.rollRange);
        
        // Pitch configuration
        imuConfig.pitchEnabled = EEPROM.read(addr++) ? true : false;
        imuConfig.pitchMidiChannel = EEPROM.read(addr++);
        imuConfig.pitchMidiCC = EEPROM.read(addr++);
        imuConfig.pitchDefaultValue = EEPROM.read(addr++);
        imuConfig.pitchToSerial = EEPROM.read(addr++) ? true : false;
        imuConfig.pitchToUSBDevice = EEPROM.read(addr++) ? true : false;
        imuConfig.pitchToUSBHost = EEPROM.read(addr++) ? true : false;
        imuConfig.pitchSensitivity = EEPROM.read(addr++) / 10.0f;
        imuConfig.pitchRange = EEPROM.read(addr++);
        
        dualPrintf("[DEBUG] Pitch loaded: en=%d, ch=%d, cc=%d, def=%d, sens=%.1f, range=%.0f\n", 
                   imuConfig.pitchEnabled, imuConfig.pitchMidiChannel, imuConfig.pitchMidiCC, 
                   imuConfig.pitchDefaultValue, imuConfig.pitchSensitivity, imuConfig.pitchRange);
        
        // Yaw configuration
        imuConfig.yawEnabled = EEPROM.read(addr++) ? true : false;
        imuConfig.yawMidiChannel = EEPROM.read(addr++);
        imuConfig.yawMidiCC = EEPROM.read(addr++);
        imuConfig.yawDefaultValue = EEPROM.read(addr++);
        imuConfig.yawToSerial = EEPROM.read(addr++) ? true : false;
        imuConfig.yawToUSBDevice = EEPROM.read(addr++) ? true : false;
        imuConfig.yawToUSBHost = EEPROM.read(addr++) ? true : false;
        imuConfig.yawSensitivity = EEPROM.read(addr++) / 10.0f;
        imuConfig.yawRange = EEPROM.read(addr++);
        
        dualPrintf("[DEBUG] Yaw loaded: en=%d, ch=%d, cc=%d, def=%d, sens=%.1f, range=%.0f\n", 
                   imuConfig.yawEnabled, imuConfig.yawMidiChannel, imuConfig.yawMidiCC, 
                   imuConfig.yawDefaultValue, imuConfig.yawSensitivity, imuConfig.yawRange);
        
        setIMUConfig(imuConfig);
        dualPrintln("[DEBUG] IMU config loaded from EEPROM");
    } else {
        // Initialize with defaults if no IMU config in EEPROM
        dualPrintf("[DEBUG] No IMU config in EEPROM, using defaults (addr=%d + 27 > %d)\n", addr, CONFIG_EEPROM_SIZE);
        resetIMUConfig();
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
    // Destination Filters
    JsonArray destFilters = doc["destFilters"].to<JsonArray>();
    for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
        JsonArray ifaceArr = destFilters.add<JsonArray>();
        for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
            ifaceArr.add(getMidiDestFilterState(iface, msg));
        }
    }
    // Channels
    JsonArray channels = doc["channels"].to<JsonArray>();
    for (int ch = 0; ch < 16; ++ch) {
        channels.add(getChannelEnabledState(ch));
    }
    // IMU configuration
    imuConfigToJson(doc);
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

    // Destination filters (optional for backward compatibility)
    JsonArray destFiltersArr = ((JsonDocument&)doc)["destFilters"].as<JsonArray>();
    if (!destFiltersArr.isNull()) {
        if (destFiltersArr.size() != MIDI_INTERFACE_COUNT) {
            Serial.print("[DEBUG] updateConfigFromJson: 'destFilters' array size is ");
            Serial.print(destFiltersArr.size());
            Serial.print(", expected ");
            Serial.println(MIDI_INTERFACE_COUNT);
            return false;
        }

        for (int iface = 0; iface < MIDI_INTERFACE_COUNT; iface++) {
            JsonArray ifaceArr = destFiltersArr[iface].as<JsonArray>();
            if (ifaceArr.isNull()) {
                Serial.print("[DEBUG] updateConfigFromJson: destFilters[");
                Serial.print(iface);
                Serial.println("] is not an array (as<JsonArray>() isNull).");
                return false;
            }
            if (ifaceArr.size() != MIDI_MSG_COUNT) {
                Serial.print("[DEBUG] updateConfigFromJson: destFilters[");
                Serial.print(iface);
                Serial.print("] size is ");
                Serial.print(ifaceArr.size());
                Serial.print(", expected ");
                Serial.println(MIDI_MSG_COUNT);
                return false;
            }
            for (int msg = 0; msg < MIDI_MSG_COUNT; msg++) {
                setMidiDestFilterState(iface, msg, ifaceArr[msg].as<bool>());
            }
        }
    } else {
        for (int iface = 0; iface < MIDI_INTERFACE_COUNT; iface++) {
            for (int msg = 0; msg < MIDI_MSG_COUNT; msg++) {
                setMidiDestFilterState(iface, msg, false);
            }
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

    // Update IMU configuration if present
    updateIMUConfigFromJson(doc);

    Serial.println("[DEBUG] updateConfigFromJson: config accepted.");
    return true;
}
