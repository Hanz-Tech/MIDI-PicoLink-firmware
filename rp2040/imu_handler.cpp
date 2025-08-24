#include "imu_handler.h"
#include "serial_midi_handler.h"
#include "usb_host_wrapper.h"
#include "midi_instances.h"
#include "serial_utils.h"
#include <Wire.h>
#include <math.h>

// IMU instance - using MPU6050 as it's in the current code
MPU6050 IMU(Wire1);
calData calib = { 0 };

// Global IMU configuration
static IMUConfig imuConfig;

// IMU calculation variables
static float roll = 0.0, pitch = 0.0, yaw = 0.0;
static float gyroXangle = 0.0, gyroYangle = 0.0, gyroZangle = 0.0;
static float compAngleX = 0.0, compAngleY = 0.0, compAngleZ = 0.0;

// Timing variables
static unsigned long lastTime = 0;
static unsigned long lastMidiTime = 0;

// Complementary filter coefficient
static const float alpha = 0.98;

// Calibration offsets
static float gyroXoffset = 0.0, gyroYoffset = 0.0, gyroZoffset = 0.0;
static float accelXoffset = 0.0, accelYoffset = 0.0, accelZoffset = 0.0;
static bool calibrated = false;

// Previous MIDI values to detect changes
static uint8_t lastRollValue = 255;
static uint8_t lastPitchValue = 255;
static uint8_t lastYawValue = 255;

bool setupIMU() {
    // Note: Do NOT reset config here - it should be loaded from EEPROM first
    // resetIMUConfig() is only called when no EEPROM data exists
    
    dualPrintf("[DEBUG] setupIMU: Current config - Roll en=%d, Pitch en=%d, Yaw en=%d\n", 
               imuConfig.rollEnabled, imuConfig.pitchEnabled, imuConfig.yawEnabled);
    
    dualPrintln("Initializing IMU...");
    
    // Configure I2C pins for Wire1
    Wire1.setSDA(IMU_I2C_SDA_PIN);
    Wire1.setSCL(IMU_I2C_SCL_PIN);
    
    // Initialize I2C
    Wire1.begin();
    Wire1.setClock(400000); // 400kHz clock
    
    // Initialize IMU
    int err = IMU.init(calib, IMU_ADDRESS);
    if (err != 0) {
        dualPrintf("Error initializing IMU: %d\n", err);
        return false;
    }
    
    dualPrintln("IMU Found!");
    
    // Configure IMU ranges
    err = IMU.setGyroRange(500);      // ±500 DPS
    err += IMU.setAccelRange(2);      // ±2g
    
    if (err != 0) {
        dualPrintf("Error setting IMU ranges: %d\n", err);
        return false;
    }
    
    dualPrintln("IMU initialized successfully");
    lastTime = millis();
    
    return true;
}

void loopIMU() {
    // Check if any axis is enabled - no need for global IMU enable anymore
    if (!imuConfig.rollEnabled && !imuConfig.pitchEnabled && !imuConfig.yawEnabled) {
        return;
    }
    
    // Update IMU angles
    float currentRoll, currentPitch, currentYaw;
    if (!getIMUAngles(currentRoll, currentPitch, currentYaw)) {
        return; // Failed to read IMU
    }
    
    // Debug: Print angles every 500ms
    static unsigned long lastDebugTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastDebugTime > 500) {
        dualPrintf("[IMU DEBUG] Roll: %.2f°, Pitch: %.2f°, Yaw: %.2f°\n", 
                   currentRoll, currentPitch, currentYaw);
        lastDebugTime = currentTime;
    }
    
    // Check if it's time to send MIDI updates
    if (currentTime - lastMidiTime < IMU_MIDI_UPDATE_RATE_MS) {
        return;
    }
    lastMidiTime = currentTime;
    
    // Convert angles to MIDI CC values and send if changed
    if (imuConfig.rollEnabled) {
        uint8_t rollValue = angleToMidiCC(currentRoll, imuConfig.rollRange, imuConfig.rollDefaultValue);
        if (rollValue != lastRollValue) {
            dualPrintf("[IMU MIDI] Roll: %.2f° -> CC%d = %d (Ch%d)\n", 
                       currentRoll, imuConfig.rollMidiCC, rollValue, imuConfig.rollMidiChannel);
            sendIMUMidiCC(imuConfig.rollMidiChannel, imuConfig.rollMidiCC, rollValue,
                         imuConfig.rollToSerial, imuConfig.rollToUSBDevice, imuConfig.rollToUSBHost);
            lastRollValue = rollValue;
        }
    }
    
    if (imuConfig.pitchEnabled) {
        uint8_t pitchValue = angleToMidiCC(currentPitch, imuConfig.pitchRange, imuConfig.pitchDefaultValue);
        if (pitchValue != lastPitchValue) {
            dualPrintf("[IMU MIDI] Pitch: %.2f° -> CC%d = %d (Ch%d)\n", 
                       currentPitch, imuConfig.pitchMidiCC, pitchValue, imuConfig.pitchMidiChannel);
            sendIMUMidiCC(imuConfig.pitchMidiChannel, imuConfig.pitchMidiCC, pitchValue,
                         imuConfig.pitchToSerial, imuConfig.pitchToUSBDevice, imuConfig.pitchToUSBHost);
            lastPitchValue = pitchValue;
        }
    }
    
    if (imuConfig.yawEnabled) {
        uint8_t yawValue = angleToMidiCC(currentYaw, imuConfig.yawRange, imuConfig.yawDefaultValue);
        if (yawValue != lastYawValue) {
            dualPrintf("[IMU MIDI] Yaw: %.2f° -> CC%d = %d (Ch%d)\n", 
                       currentYaw, imuConfig.yawMidiCC, yawValue, imuConfig.yawMidiChannel);
            sendIMUMidiCC(imuConfig.yawMidiChannel, imuConfig.yawMidiCC, yawValue,
                         imuConfig.yawToSerial, imuConfig.yawToUSBDevice, imuConfig.yawToUSBHost);
            lastYawValue = yawValue;
        }
    }
}

bool getIMUAngles(float &roll, float &pitch, float &yaw) {
    // Read sensor data
    AccelData accelData;
    GyroData gyroData;
    
    IMU.update();
    IMU.getAccel(&accelData);
    IMU.getGyro(&gyroData);
    
    // Get current time and calculate elapsed time
    unsigned long currentTime = millis();
    float elapsedTime = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;
    
    // Apply calibration offsets
    float gyroX = (gyroData.gyroX - gyroXoffset) * imuConfig.rollSensitivity;
    float gyroY = (gyroData.gyroY - gyroYoffset) * imuConfig.pitchSensitivity;
    float gyroZ = (gyroData.gyroZ - gyroZoffset) * imuConfig.yawSensitivity;
    
    float accelX = accelData.accelX - accelXoffset;
    float accelY = accelData.accelY - accelYoffset;
    float accelZ = accelData.accelZ - accelZoffset;
    
    // Calculate angles from accelerometer
    float accelAngleX = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
    float accelAngleY = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
    
    // Calculate angles from gyroscope (integration)
    gyroXangle += gyroX * elapsedTime;
    gyroYangle += gyroY * elapsedTime;
    gyroZangle += gyroZ * elapsedTime;
    
    // Apply complementary filter
    compAngleX = alpha * (compAngleX + gyroX * elapsedTime) + (1.0 - alpha) * accelAngleX;
    compAngleY = alpha * (compAngleY + gyroY * elapsedTime) + (1.0 - alpha) * accelAngleY;
    compAngleZ += gyroZ * elapsedTime; // Yaw uses only gyroscope
    
    // Set output values
    roll = compAngleX;
    pitch = compAngleY;
    yaw = compAngleZ;
    
    return true;
}

void calibrateIMU() {
    dualPrintln("Calibrating IMU... Keep the device flat and still!");
    dualPrintln("Starting calibration in 3 seconds...");
    delay(3000);
    
    const int calibrationSamples = 100;
    float gyroXsum = 0, gyroYsum = 0, gyroZsum = 0;
    float accelXsum = 0, accelYsum = 0, accelZsum = 0;
    
    for (int i = 0; i < calibrationSamples; i++) {
        AccelData accelData;
        GyroData gyroData;
        
        IMU.update();
        IMU.getAccel(&accelData);
        IMU.getGyro(&gyroData);
        
        gyroXsum += gyroData.gyroX;
        gyroYsum += gyroData.gyroY;
        gyroZsum += gyroData.gyroZ;
        
        accelXsum += accelData.accelX;
        accelYsum += accelData.accelY;
        accelZsum += accelData.accelZ - 1.0;  // Subtract 1g for Z axis
        
        delay(10);
    }
    
    // Calculate averages for offset
    gyroXoffset = gyroXsum / calibrationSamples;
    gyroYoffset = gyroYsum / calibrationSamples;
    gyroZoffset = gyroZsum / calibrationSamples;
    
    accelXoffset = accelXsum / calibrationSamples;
    accelYoffset = accelYsum / calibrationSamples;
    accelZoffset = accelZsum / calibrationSamples;
    
    calibrated = true;
    dualPrintln("IMU calibration complete!");
}

uint8_t angleToMidiCC(float angle, float range, uint8_t defaultValue) {
    // Clamp angle to range
    if (angle > range) angle = range;
    if (angle < -range) angle = -range;
    
    // Convert angle to 0-127 range with default value as center
    float normalized = (angle / range) * 63.5f; // ±63.5 range
    int midiValue = defaultValue + (int)normalized;
    
    // Clamp to MIDI CC range
    if (midiValue < 0) midiValue = 0;
    if (midiValue > 127) midiValue = 127;
    
    return (uint8_t)midiValue;
}

void sendIMUMidiCC(uint8_t channel, uint8_t cc, uint8_t value, bool toSerial, bool toUSBDevice, bool toUSBHost) {
    // Send to Serial MIDI
    if (toSerial) {
        sendSerialMidiControlChange(channel, cc, value);
    }
    
    // Send to USB Device MIDI
    if (toUSBDevice) {
        USB_D.sendControlChange(cc, value, channel);
    }
    
    // Send to USB Host MIDI  
    if (toUSBHost && midi_host_mounted) {
        sendControlChange(channel, cc, value);
    }
}

void setIMUConfig(const IMUConfig &config) {
    imuConfig = config;
}

IMUConfig getIMUConfig() {
    return imuConfig;
}

void resetIMUConfig() {
    // Note: No global enabled flag - determined automatically by individual axis enables
    
    // Roll configuration
    imuConfig.rollEnabled = false;
    imuConfig.rollMidiChannel = DEFAULT_MIDI_CHANNEL;
    imuConfig.rollMidiCC = DEFAULT_ROLL_CC;
    imuConfig.rollDefaultValue = DEFAULT_CC_VALUE;
    imuConfig.rollToSerial = true;
    imuConfig.rollToUSBDevice = true;
    imuConfig.rollToUSBHost = true;
    imuConfig.rollSensitivity = DEFAULT_SENSITIVITY;
    imuConfig.rollRange = DEFAULT_RANGE;
    
    // Pitch configuration
    imuConfig.pitchEnabled = false;
    imuConfig.pitchMidiChannel = DEFAULT_MIDI_CHANNEL;
    imuConfig.pitchMidiCC = DEFAULT_PITCH_CC;
    imuConfig.pitchDefaultValue = DEFAULT_CC_VALUE;
    imuConfig.pitchToSerial = true;
    imuConfig.pitchToUSBDevice = true;
    imuConfig.pitchToUSBHost = true;
    imuConfig.pitchSensitivity = DEFAULT_SENSITIVITY;
    imuConfig.pitchRange = DEFAULT_RANGE;
    
    // Yaw configuration
    imuConfig.yawEnabled = false;
    imuConfig.yawMidiChannel = DEFAULT_MIDI_CHANNEL;
    imuConfig.yawMidiCC = DEFAULT_YAW_CC;
    imuConfig.yawDefaultValue = DEFAULT_CC_VALUE;
    imuConfig.yawToSerial = true;
    imuConfig.yawToUSBDevice = true;
    imuConfig.yawToUSBHost = true;
    imuConfig.yawSensitivity = DEFAULT_SENSITIVITY;
    imuConfig.yawRange = DEFAULT_RANGE;
}

void imuConfigToJson(JsonDocument& doc) {
    JsonObject imu = doc["imu"].to<JsonObject>();
    
    // Note: No global enabled field - determined automatically by individual axis enables
    
    // Roll configuration
    JsonObject roll = imu["roll"].to<JsonObject>();
    roll["enabled"] = imuConfig.rollEnabled;
    roll["channel"] = imuConfig.rollMidiChannel;
    roll["cc"] = imuConfig.rollMidiCC;
    roll["defaultValue"] = imuConfig.rollDefaultValue;
    roll["toSerial"] = imuConfig.rollToSerial;
    roll["toUSBDevice"] = imuConfig.rollToUSBDevice;
    roll["toUSBHost"] = imuConfig.rollToUSBHost;
    roll["sensitivity"] = imuConfig.rollSensitivity;
    roll["range"] = imuConfig.rollRange;
    
    // Pitch configuration
    JsonObject pitch = imu["pitch"].to<JsonObject>();
    pitch["enabled"] = imuConfig.pitchEnabled;
    pitch["channel"] = imuConfig.pitchMidiChannel;
    pitch["cc"] = imuConfig.pitchMidiCC;
    pitch["defaultValue"] = imuConfig.pitchDefaultValue;
    pitch["toSerial"] = imuConfig.pitchToSerial;
    pitch["toUSBDevice"] = imuConfig.pitchToUSBDevice;
    pitch["toUSBHost"] = imuConfig.pitchToUSBHost;
    pitch["sensitivity"] = imuConfig.pitchSensitivity;
    pitch["range"] = imuConfig.pitchRange;
    
    // Yaw configuration
    JsonObject yaw = imu["yaw"].to<JsonObject>();
    yaw["enabled"] = imuConfig.yawEnabled;
    yaw["channel"] = imuConfig.yawMidiChannel;
    yaw["cc"] = imuConfig.yawMidiCC;
    yaw["defaultValue"] = imuConfig.yawDefaultValue;
    yaw["toSerial"] = imuConfig.yawToSerial;
    yaw["toUSBDevice"] = imuConfig.yawToUSBDevice;
    yaw["toUSBHost"] = imuConfig.yawToUSBHost;
    yaw["sensitivity"] = imuConfig.yawSensitivity;
    yaw["range"] = imuConfig.yawRange;
}

bool updateIMUConfigFromJson(const JsonDocument& doc) {
    JsonObject imu = ((JsonDocument&)doc)["imu"].as<JsonObject>();
    if (imu.isNull()) {
        return true; // IMU config is optional
    }
    
    // Note: No global enable/disable - determined automatically by individual axis enables
    
    // Roll configuration
    JsonObject roll = imu["roll"].as<JsonObject>();
    if (!roll.isNull()) {
        if (roll.containsKey("enabled")) imuConfig.rollEnabled = roll["enabled"].as<bool>();
        if (roll.containsKey("channel")) imuConfig.rollMidiChannel = roll["channel"].as<uint8_t>();
        if (roll.containsKey("cc")) imuConfig.rollMidiCC = roll["cc"].as<uint8_t>();
        if (roll.containsKey("defaultValue")) imuConfig.rollDefaultValue = roll["defaultValue"].as<uint8_t>();
        if (roll.containsKey("toSerial")) imuConfig.rollToSerial = roll["toSerial"].as<bool>();
        if (roll.containsKey("toUSBDevice")) imuConfig.rollToUSBDevice = roll["toUSBDevice"].as<bool>();
        if (roll.containsKey("toUSBHost")) imuConfig.rollToUSBHost = roll["toUSBHost"].as<bool>();
        if (roll.containsKey("sensitivity")) imuConfig.rollSensitivity = roll["sensitivity"].as<float>();
        if (roll.containsKey("range")) imuConfig.rollRange = roll["range"].as<float>();
    }
    
    // Pitch configuration
    JsonObject pitch = imu["pitch"].as<JsonObject>();
    if (!pitch.isNull()) {
        if (pitch.containsKey("enabled")) imuConfig.pitchEnabled = pitch["enabled"].as<bool>();
        if (pitch.containsKey("channel")) imuConfig.pitchMidiChannel = pitch["channel"].as<uint8_t>();
        if (pitch.containsKey("cc")) imuConfig.pitchMidiCC = pitch["cc"].as<uint8_t>();
        if (pitch.containsKey("defaultValue")) imuConfig.pitchDefaultValue = pitch["defaultValue"].as<uint8_t>();
        if (pitch.containsKey("toSerial")) imuConfig.pitchToSerial = pitch["toSerial"].as<bool>();
        if (pitch.containsKey("toUSBDevice")) imuConfig.pitchToUSBDevice = pitch["toUSBDevice"].as<bool>();
        if (pitch.containsKey("toUSBHost")) imuConfig.pitchToUSBHost = pitch["toUSBHost"].as<bool>();
        if (pitch.containsKey("sensitivity")) imuConfig.pitchSensitivity = pitch["sensitivity"].as<float>();
        if (pitch.containsKey("range")) imuConfig.pitchRange = pitch["range"].as<float>();
    }
    
    // Yaw configuration
    JsonObject yaw = imu["yaw"].as<JsonObject>();
    if (!yaw.isNull()) {
        if (yaw.containsKey("enabled")) imuConfig.yawEnabled = yaw["enabled"].as<bool>();
        if (yaw.containsKey("channel")) imuConfig.yawMidiChannel = yaw["channel"].as<uint8_t>();
        if (yaw.containsKey("cc")) imuConfig.yawMidiCC = yaw["cc"].as<uint8_t>();
        if (yaw.containsKey("defaultValue")) imuConfig.yawDefaultValue = yaw["defaultValue"].as<uint8_t>();
        if (yaw.containsKey("toSerial")) imuConfig.yawToSerial = yaw["toSerial"].as<bool>();
        if (yaw.containsKey("toUSBDevice")) imuConfig.yawToUSBDevice = yaw["toUSBDevice"].as<bool>();
        if (yaw.containsKey("toUSBHost")) imuConfig.yawToUSBHost = yaw["toUSBHost"].as<bool>();
        if (yaw.containsKey("sensitivity")) imuConfig.yawSensitivity = yaw["sensitivity"].as<float>();
        if (yaw.containsKey("range")) imuConfig.yawRange = yaw["range"].as<float>();
    }
    
    dualPrintln("[DEBUG] IMU config updated from JSON");
    return true;
}
