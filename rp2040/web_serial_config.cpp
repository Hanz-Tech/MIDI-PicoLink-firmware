#include "web_serial_config.h"
#include "config.h"
#include "led_utils.h"
#include "version.h"
#include "midi_filters.h"
#include <ArduinoJson.h>
#include <Arduino.h>

// Delayed EEPROM save mechanism
static bool pendingEEPROMSave = false;
static uint32_t eepromSaveTime = 0;
static const uint32_t EEPROM_SAVE_DELAY_MS = 3000; // 3 seconds delay

void processWebSerialConfig() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue; // Skip empty lines

        // Serial.print("[DEBUG] Received line: ");
        // Serial.println(line);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, line);
        if (error) {
            Serial.print("{\"status\":\"deserializeJson() failed\",\"error\":\"");
            Serial.print(error.c_str());
            Serial.println("\"}");
            continue;
        }
        // Print parsed keys for debugging
        // Serial.print("[DEBUG] Parsed keys: ");
        // for (JsonPair kv : doc.as<JsonObject>()) {
        //     Serial.print(kv.key().c_str());
        //     Serial.print(" ");
        // }
        // Serial.println();

        String command = doc["command"] | "";
        if (command == "READALL") {
            JsonDocument outDoc;
            configToJson(outDoc);
            outDoc["version"] = FIRMWARE_VERSION;
            serializeJson(outDoc, Serial);
            Serial.println();
        } else if (command == "SAVEALL") {
            if (updateConfigFromJson(doc)) {
                Serial.println("{\"debug\":\"Config applied in memory\"}");
                                
                // Schedule delayed EEPROM save to avoid USB Host interference
                pendingEEPROMSave = true;
                eepromSaveTime = millis() + EEPROM_SAVE_DELAY_MS;
                Serial.println("{\"debug\":\"EEPROM save scheduled for 3 seconds\"}");
                
                // Flash both LEDs 5 times quickly using led_utils
                blinkBothLEDs(2, 100);

                Serial.println("{\"status\":\"Success\",\"command\":\"SAVEALL\",\"message\":\"Config applied immediately, saving to EEPROM in 3s\"}");
            } else {
                Serial.println("{\"status\":\"Invalid config JSON\",\"command\":\"SAVEALL\"}");
            }
        } else {
            Serial.print("{\"status\":\"Unknown command\",\"command\":\"");
            Serial.print(command);
            Serial.println("\"}");
        }
    }
}

// Call this function regularly from the main loop to handle delayed EEPROM saves
void handleDelayedEEPROMSave() {
    if (pendingEEPROMSave && millis() >= eepromSaveTime) {
        Serial.println("{\"debug\":\"Starting delayed EEPROM save...\"}");
        saveConfigToEEPROM();
        Serial.println("{\"debug\":\"Delayed EEPROM save complete\"}");
        pendingEEPROMSave = false;
    }
}
