#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <Arduino.h>

// LED pin definitions
#define LED_IN_PIN 29
#define LED_OUT_PIN 19

// Function declarations for LED handling
void initLEDs();
void handleLEDs();
void triggerSerialLED();  // Triggers LED for serial MIDI activity
void triggerUsbLED();     // Triggers LED for USB MIDI activity (host and device)

// Blink both LEDs N times with ms delay
void blinkBothLEDs(int times, int ms);

#endif // LED_UTILS_H
