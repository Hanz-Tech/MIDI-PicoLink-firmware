#include "eeprom_config.h"
#include "midi_filters.h"

// Magic bytes to identify valid configuration
const uint8_t MAGIC_BYTES[] = {'M', 'I', 'D', 'I'};
const int MAGIC_BYTES_LEN = 4;

// Structure for EEPROM header
struct EEPROMHeader {
    uint8_t magic[4];      // Magic bytes to identify valid configuration
    uint8_t version;       // Configuration version
    uint16_t dataSize;     // Size of the data following the header
    uint16_t checksum;     // Simple checksum for data validation
};

void setupEEPROM() {
    // Initialize EEPROM with the specified size
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("EEPROM: Initialized with size " + String(EEPROM_SIZE) + " bytes");
    
    // Try to load settings from EEPROM
    if (loadFilterSettings()) {
        Serial.println("EEPROM: Filter settings loaded successfully");
    } else {
        Serial.println("EEPROM: No valid settings found, using defaults");
        // Save default settings to EEPROM
        saveFilterSettings();
    }
}

// Calculate a simple checksum for data validation
uint16_t calculateChecksum(const uint8_t* data, size_t length) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

bool loadFilterSettings() {
    // Read header
    EEPROMHeader header;
    for (int i = 0; i < sizeof(header); i++) {
        ((uint8_t*)&header)[i] = EEPROM.read(i);
    }
    
    // Check magic bytes
    for (int i = 0; i < MAGIC_BYTES_LEN; i++) {
        if (header.magic[i] != MAGIC_BYTES[i]) {
            Serial.println("EEPROM: Invalid magic bytes");
            return false;
        }
    }
    
    // Check version
    if (header.version != CONFIG_VERSION) {
        Serial.println("EEPROM: Version mismatch");
        return false;
    }
    
    // Read filter data
    MidiFilterConfig config;
    int dataOffset = sizeof(header);
    for (int i = 0; i < sizeof(config); i++) {
        ((uint8_t*)&config)[i] = EEPROM.read(dataOffset + i);
    }
    
    // Calculate and verify checksum
    uint16_t calculatedChecksum = calculateChecksum((uint8_t*)&config, sizeof(config));
    if (calculatedChecksum != header.checksum) {
        Serial.println("EEPROM: Checksum mismatch");
        return false;
    }
    
    // Apply filter settings
    for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            midiFilters[interface][msgType] = config.filters[interface][msgType];
        }
    }
    
    Serial.println("EEPROM: Filter settings loaded");
    return true;
}

bool saveFilterSettings() {
    // Prepare header
    EEPROMHeader header;
    for (int i = 0; i < MAGIC_BYTES_LEN; i++) {
        header.magic[i] = MAGIC_BYTES[i];
    }
    header.version = CONFIG_VERSION;
    header.dataSize = sizeof(MidiFilterConfig);
    
    // Prepare filter data
    MidiFilterConfig config;
    for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
            config.filters[interface][msgType] = midiFilters[interface][msgType];
        }
    }
    
    // Calculate checksum
    header.checksum = calculateChecksum((uint8_t*)&config, sizeof(config));
    
    // Write header to EEPROM
    int offset = 0;
    for (int i = 0; i < sizeof(header); i++) {
        EEPROM.write(offset + i, ((uint8_t*)&header)[i]);
    }
    
    // Write filter data to EEPROM
    offset += sizeof(header);
    for (int i = 0; i < sizeof(config); i++) {
        EEPROM.write(offset + i, ((uint8_t*)&config)[i]);
    }
    
    // Commit changes to flash
    bool success = EEPROM.commit();
    if (success) {
        Serial.println("EEPROM: Filter settings saved successfully");
    } else {
        Serial.println("EEPROM: Failed to save filter settings");
    }
    
    return success;
}

String exportFilterSettingsJson() {
    // Create a JSON document
    DynamicJsonDocument doc(1024);
    
    // Add version information
    doc["version"] = CONFIG_VERSION;
    
    // Create arrays for each interface
    JsonArray serialFilters = doc.createNestedArray("serial");
    JsonArray usbDeviceFilters = doc.createNestedArray("usbDevice");
    JsonArray usbHostFilters = doc.createNestedArray("usbHost");
    
    // Message type names for JSON keys
    const char* msgTypeNames[] = {
        "noteOn", "noteOff", "polyAftertouch", "controlChange", 
        "programChange", "channelAftertouch", "pitchBend", 
        "sysEx", "realtime"
    };
    
    // Add filter settings for each interface and message type
    for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
        serialFilters.add(midiFilters[MIDI_INTERFACE_SERIAL][msgType]);
        usbDeviceFilters.add(midiFilters[MIDI_INTERFACE_USB_DEVICE][msgType]);
        usbHostFilters.add(midiFilters[MIDI_INTERFACE_USB_HOST][msgType]);
    }
    
    // Serialize JSON to string
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool importFilterSettingsJson(const String& jsonString) {
    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    
    // Check for parsing errors
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Check version
    if (!doc.containsKey("version") || doc["version"] != CONFIG_VERSION) {
        Serial.println("JSON: Version mismatch or missing");
        return false;
    }
    
    // Check if all required arrays exist
    if (!doc.containsKey("serial") || !doc.containsKey("usbDevice") || !doc.containsKey("usbHost")) {
        Serial.println("JSON: Missing interface arrays");
        return false;
    }
    
    // Get filter arrays
    JsonArray serialFilters = doc["serial"];
    JsonArray usbDeviceFilters = doc["usbDevice"];
    JsonArray usbHostFilters = doc["usbHost"];
    
    // Check array sizes
    if (serialFilters.size() != MIDI_MSG_COUNT || 
        usbDeviceFilters.size() != MIDI_MSG_COUNT || 
        usbHostFilters.size() != MIDI_MSG_COUNT) {
        Serial.println("JSON: Array size mismatch");
        return false;
    }
    
    // Apply filter settings
    for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
        midiFilters[MIDI_INTERFACE_SERIAL][msgType] = serialFilters[msgType];
        midiFilters[MIDI_INTERFACE_USB_DEVICE][msgType] = usbDeviceFilters[msgType];
        midiFilters[MIDI_INTERFACE_USB_HOST][msgType] = usbHostFilters[msgType];
    }
    
    // Save to EEPROM
    bool success = saveFilterSettings();
    
    return success;
}

void processSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Check if it's a JSON command
        if (command.startsWith("{") && command.endsWith("}")) {
            // Parse JSON command
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, command);
            
            if (error) {
                Serial.print("JSON parsing failed: ");
                Serial.println(error.c_str());
                return;
            }
            
            // Check for command type
            if (doc.containsKey("command")) {
                String cmdType = doc["command"];
                
                if (cmdType == "getFilters") {
                    // Return current filter settings as JSON
                    Serial.println(exportFilterSettingsJson());
                }
                else if (cmdType == "setFilters" && doc.containsKey("filters")) {
                    // Import filter settings from JSON
                    String filterJson;
                    serializeJson(doc["filters"], filterJson);
                    if (importFilterSettingsJson(filterJson)) {
                        Serial.println("{\"status\":\"success\",\"message\":\"Filter settings updated\"}");
                    } else {
                        Serial.println("{\"status\":\"error\",\"message\":\"Failed to update filter settings\"}");
                    }
                }
                else if (cmdType == "resetFilters") {
                    // Reset all filters to default (disabled)
                    for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
                        for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
                            midiFilters[interface][msgType] = false;
                        }
                    }
                    
                    if (saveFilterSettings()) {
                        Serial.println("{\"status\":\"success\",\"message\":\"Filter settings reset to defaults\"}");
                    } else {
                        Serial.println("{\"status\":\"error\",\"message\":\"Failed to reset filter settings\"}");
                    }
                }
                else {
                    Serial.println("{\"status\":\"error\",\"message\":\"Unknown command\"}");
                }
            }
            else {
                // If it's a filter configuration without a command, try to import it directly
                if (importFilterSettingsJson(command)) {
                    Serial.println("{\"status\":\"success\",\"message\":\"Filter settings updated\"}");
                } else {
                    Serial.println("{\"status\":\"error\",\"message\":\"Failed to update filter settings\"}");
                }
            }
        }
        else if (command == "get_filters") {
            // Legacy command for getting filter settings
            Serial.println(exportFilterSettingsJson());
        }
        else if (command == "reset_filters") {
            // Legacy command for resetting filters
            for (int interface = 0; interface < MIDI_INTERFACE_COUNT; interface++) {
                for (int msgType = 0; msgType < MIDI_MSG_COUNT; msgType++) {
                    midiFilters[interface][msgType] = false;
                }
            }
            
            if (saveFilterSettings()) {
                Serial.println("Filter settings reset to defaults");
            } else {
                Serial.println("Failed to reset filter settings");
            }
        }
    }
}
