#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ============================================
// MIDI PicoLink — Centralized Pin Configuration
// ============================================
// All GPIO pin assignments in one place.
// See schematic for physical connections.

// --- USB Host (PIO USB) ---
#define HOST_PIN_DP 12  // D+ pin for PIO USB Host (D- = D+ + 1 = GPIO 13)

// --- Serial MIDI (DIN/TRS via Serial1) ---
#define SERIAL_MIDI_RX_PIN 1  // GPIO pin for Serial1 RX (MIDI In)
#define SERIAL_MIDI_TX_PIN 0  // GPIO pin for Serial1 TX (MIDI Out)

// --- LED Indicators ---
#define LED_IN_PIN 29   // LED for incoming MIDI activity (USB)
#define LED_OUT_PIN 19  // LED for outgoing MIDI activity (Serial)

// --- IMU (I2C via Wire1) ---
#define IMU_I2C_SDA_PIN 26  // I2C SDA for IMU sensor
#define IMU_I2C_SCL_PIN 27  // I2C SCL for IMU sensor

// --- Debug UART (Serial2) ---
#define DEBUG_UART_RX_PIN 25  // Serial2 RX for debug output
#define DEBUG_UART_TX_PIN 24  // Serial2 TX for debug output

#endif  // PIN_CONFIG_H
