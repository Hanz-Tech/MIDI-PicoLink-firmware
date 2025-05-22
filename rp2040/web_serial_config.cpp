#include "web_serial_config.h"
#include "config.h"
#include "led_utils.h"
#include <ArduinoJson.h>
#include <Arduino.h>

void processWebSerialConfig() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue; // Skip empty lines

        Serial.print("[DEBUG] Received line: ");
        Serial.println(line);

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, line);
        if (error) {
            Serial.print("{\"status\":\"deserializeJson() failed\",\"error\":\"");
            Serial.print(error.c_str());
            Serial.println("\"}");
            continue;
        }
        // Print parsed keys for debugging
        Serial.print("[DEBUG] Parsed keys: ");
        for (JsonPair kv : doc.as<JsonObject>()) {
            Serial.print(kv.key().c_str());
            Serial.print(" ");
        }
        Serial.println();

        String command = doc["command"] | "";
        if (command == "READALL") {
            JsonDocument outDoc;
            configToJson(outDoc);
            serializeJson(outDoc, Serial);
            Serial.println();
        } else if (command == "SAVEALL") {
            if (updateConfigFromJson(doc)) {
                saveConfigToEEPROM();
                loadConfigFromEEPROM(); // Ensure in-memory state matches EEPROM

                // Flash both LEDs 5 times quickly using led_utils
                for (int i = 0; i < 5; i++) {
                    triggerSerialLED();
                    triggerUsbLED();
                    delay(100);
                }

                Serial.println("{\"status\":\"Success\",\"command\":\"SAVEALL\"}");
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
