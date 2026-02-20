#include "led_utils.h"

// Shared variables for LED state
static uint32_t inLedStartMs = 0;
static bool inLedActive = false;
static uint32_t outLedStartMs = 0;
static bool outLedActive = false;

// Blink scheduler state
static bool blinkActive = false;
static bool blinkLedOn = false;
static uint8_t blinkRemainingTransitions = 0;
static uint16_t blinkIntervalMs = 0;
static uint32_t blinkNextStepAt = 0;

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

  // Blink scheduler runs alongside activity timers (50ms behavior unchanged).
  if (blinkActive && millis() >= blinkNextStepAt) {
    blinkLedOn = !blinkLedOn;
    digitalWrite(LED_IN_PIN, blinkLedOn ? HIGH : LOW);
    digitalWrite(LED_OUT_PIN, blinkLedOn ? HIGH : LOW);
    blinkNextStepAt = millis() + blinkIntervalMs;
    if (!blinkLedOn && blinkRemainingTransitions > 0) {
      blinkRemainingTransitions--;
      if (blinkRemainingTransitions == 0) {
        blinkActive = false;
        digitalWrite(LED_IN_PIN, LOW);
        digitalWrite(LED_OUT_PIN, LOW);
      }
    }
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

// Blink both LEDs N times with ms delay (synchronous/blocking)
void blinkBothLEDs(int times, int ms) {
  if (times <= 0 || ms <= 0) {
    blinkActive = false;
    blinkRemainingTransitions = 0;
    digitalWrite(LED_IN_PIN, LOW);
    digitalWrite(LED_OUT_PIN, LOW);
    return;
  }

  blinkActive = true;
  blinkLedOn = false;
  blinkRemainingTransitions = static_cast<uint8_t>(times * 2);
  blinkIntervalMs = static_cast<uint16_t>(ms);
  blinkNextStepAt = millis();
}
