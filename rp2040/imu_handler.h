#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include <Arduino.h>
#include "pin_config.h"
#include "FastIMU.h"
#include <ArduinoJson.h>

// IMU Configuration Structure
typedef struct {
    // Roll axis configuration
    bool rollEnabled;
    uint8_t rollMidiChannel;     // 1-16
    uint8_t rollMidiCC;          // 0-127
    uint8_t rollDefaultValue;    // 0-127 (value when device is flat)
    bool rollToSerial;
    bool rollToUSBDevice;
    bool rollToUSBHost;
    
    // Pitch axis configuration
    bool pitchEnabled;
    uint8_t pitchMidiChannel;    // 1-16
    uint8_t pitchMidiCC;         // 0-127
    uint8_t pitchDefaultValue;   // 0-127 (value when device is flat)
    bool pitchToSerial;
    bool pitchToUSBDevice;
    bool pitchToUSBHost;
    
    // Yaw axis configuration
    bool yawEnabled;
    uint8_t yawMidiChannel;      // 1-16
    uint8_t yawMidiCC;           // 0-127
    uint8_t yawDefaultValue;     // 0-127 (value when device is flat)
    bool yawToSerial;
    bool yawToUSBDevice;
    bool yawToUSBHost;
    
    // Sensitivity settings
    float rollSensitivity;       // Multiplier for roll angle
    float pitchSensitivity;      // Multiplier for pitch angle
    float yawSensitivity;        // Multiplier for yaw angle
    
    // Range settings (degrees)
    float rollRange;             // ±range in degrees that maps to 0-127
    float pitchRange;            // ±range in degrees that maps to 0-127
    float yawRange;              // ±range in degrees that maps to 0-127
    
} IMUConfig;

// Function declarations
bool setupIMU();
void loopIMU();
void calibrateIMU();
void startIMUCalibration();
void updateIMUCalibration();
bool isIMUCalibrationActive();
bool getIMUAngles(float &roll, float &pitch, float &yaw);
uint8_t angleToMidiCC(float angle, float range, uint8_t defaultValue);
void sendIMUMidiCC(uint8_t channel, uint8_t cc, uint8_t value, bool toSerial, bool toUSBDevice, bool toUSBHost);

// Configuration functions
void setIMUConfig(const IMUConfig &config);
IMUConfig getIMUConfig();
void resetIMUConfig();

// JSON serialization
void imuConfigToJson(JsonDocument& doc);
bool updateIMUConfigFromJson(const JsonDocument& doc);

// Default configuration values
#define DEFAULT_MIDI_CHANNEL 1
#define DEFAULT_ROLL_CC 1
#define DEFAULT_PITCH_CC 2
#define DEFAULT_YAW_CC 3
#define DEFAULT_CC_VALUE 64
#define DEFAULT_SENSITIVITY 1.0f
#define DEFAULT_RANGE 45.0f

// I2C pin configuration
#define IMU_ADDRESS 0x68  // MPU6050/LSM6DS3 address (alt: 0x6B for MPU6050, 0x6A/0x6B for LSM6DS3)

// Update rate for MIDI CC messages (to avoid flooding)
#define IMU_MIDI_UPDATE_RATE_MS 50  // 20Hz update rate

#endif // IMU_HANDLER_H
