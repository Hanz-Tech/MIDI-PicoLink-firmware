#include "led_utils.h"

// Shared variables for LED state
static uint32_t inLedStartMs = 0;
static bool inLedActive = false;
static uint32_t outLedStartMs = 0;
static bool outLedActive = false;

// Initialize LED pins
void initLEDs() {
  pinMode(LED_IN_PIN, OUTPUT);
  pinMode(LED_OUT_PIN, OUTPUT);
  digitalWrite(LED_IN_PIN, LOW);
  digitalWrite(LED_OUT_PIN, LOW);
}

// LED handling functions for MIDI activity
void handleLEDs() {
  // Handle LED_IN_PIN (for incoming serial MIDI)
  // If LED is active and 50ms has passed, turn it off
  if (inLedActive && (millis() - inLedStartMs >= 50)) {
    digitalWrite(LED_IN_PIN, LOW);
    inLedActive = false;
  }
  
  // Handle LED_OUT_PIN (for outgoing serial MIDI)
  // If LED is active and 50ms has passed, turn it off
  if (outLedActive && (millis() - outLedStartMs >= 50)) {
    digitalWrite(LED_OUT_PIN, LOW);
    outLedActive = false;
  }
}

// Function to trigger the serial MIDI LED for 50ms
void triggerSerialLED() {
  // Turn on LED and record the time
  digitalWrite(LED_IN_PIN, HIGH);
  inLedStartMs = millis();
  inLedActive = true;
}

// Function to trigger the USB MIDI LED for 50ms
void triggerUsbLED() {
  // Turn on LED and record the time
  digitalWrite(LED_OUT_PIN, HIGH);
  outLedStartMs = millis();
  outLedActive = true;
}
