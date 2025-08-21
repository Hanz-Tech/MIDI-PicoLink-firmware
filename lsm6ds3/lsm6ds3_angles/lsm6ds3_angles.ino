/*
 * LSM6DS3 Roll, Pitch, and Yaw Angle Calculation
 * Based on the FastIMU library and adapted from MPU6050 angle calculation
 * 
 * This sketch calculates and displays the roll, pitch, and yaw angles
 * from LSM6DS3 accelerometer and gyroscope data using complementary filtering.
 */

#include "FastIMU.h"
#include <Wire.h>
#include <math.h>

// #define IMU_ADDRESS 0x6B    // LSM6DS3 I2C address
// LSM6DS3 IMU(Wire1);               // LSM6DS3 IMU instance

#define IMU_ADDRESS 0x68    // LSM6DS3 I2C address
MPU6050 IMU(Wire1);         // MPU6050 IMU instance

// Variables for angle calculation
float roll = 0.0, pitch = 0.0, yaw = 0.0;
float gyroXangle = 0.0, gyroYangle = 0.0, gyroZangle = 0.0;
float compAngleX = 0.0, compAngleY = 0.0, compAngleZ = 0.0;

// Sensitivity multiplier for yaw detection
const float yawSensitivity = 1.0;  // Adjust based on your rotation speed

// Timing variables
unsigned long lastTime = 0;
unsigned long currentTime = 0;
float elapsedTime = 0.0;

// Complementary filter coefficient (0.98 for gyro, 0.02 for accel)
const float alpha = 0.98;

// Offset variables for calibration
float gyroXoffset = 0.0, gyroYoffset = 0.0, gyroZoffset = 0.0;
float accelXoffset = 0.0, accelYoffset = 0.0, accelZoffset = 0.0;

// Calibration samples
const int calibrationSamples = 100;
bool calibrated = false;

// Configure I2C pins for RP2040 - Wire1 custom pins
const int I2C1_SDA_PIN = 26;  // Custom SDA pin for Wire1
const int I2C1_SCL_PIN = 27;  // Custom SCL pin for Wire1

calData calib = { 0 };  // Calibration data from FastIMU

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("LSM6DS3 Angle Calculation Demo");
  Serial.println("==============================");

  // Configure I2C pins for Wire1
  Wire1.setSDA(I2C1_SDA_PIN);
  Wire1.setSCL(I2C1_SCL_PIN);
  
  // Initialize I2C with Wire1
  Wire1.begin();
  Wire1.setClock(400000); // 400kHz clock

  // Initialize LSM6DS3
  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) {
    Serial.print("Error initializing LSM6DS3: ");
    Serial.println(err);
    while (true) {
      ;
    }
  }

  Serial.println("LSM6DS3 Found!");
  
  // Configure LSM6DS3
  err = IMU.setGyroRange(500);      // ±500 DPS
  err = IMU.setAccelRange(2);       // ±2g
  
  if (err != 0) {
    Serial.print("Error setting ranges: ");
    Serial.println(err);
    while (true) {
      ;
    }
  }
  
  Serial.println("Calibrating sensors... Keep the sensor flat and still!");
  Serial.println("Starting calibration in 3 seconds...");
  delay(3000);
  
  // Calibrate sensors
  calibrateSensors();
  
  Serial.println("Calibration complete!");
  Serial.println("Roll\tPitch\tYaw");
  Serial.println("(deg)\t(deg)\t(deg)");
  Serial.println("--------------------------------");
}

void loop() {
  // Read sensor data
  AccelData accelData;
  GyroData gyroData;
  
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);
  
  // Get current time
  currentTime = millis();
  elapsedTime = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;
  
  // Apply calibration offsets
  float gyroX = gyroData.gyroX - gyroXoffset;
  float gyroY = gyroData.gyroY - gyroYoffset;
  float gyroZ = gyroData.gyroZ - gyroZoffset;
  
  float accelX = accelData.accelX - accelXoffset;
  float accelY = accelData.accelY - accelYoffset;
  float accelZ = accelData.accelZ - accelZoffset;
  
  // Calculate angles from accelerometer (convert from g to m/s² if needed)
  // FastIMU provides values in g, so we use them directly
  float accelAngleX = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
  float accelAngleY = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
  
  // Calculate angles from gyroscope (integration)
  gyroXangle += gyroX * elapsedTime;
  gyroYangle += gyroY * elapsedTime;
  gyroZangle += gyroZ * elapsedTime;
  
  // Apply complementary filter to combine accelerometer and gyroscope data
  compAngleX = alpha * (compAngleX + gyroX * elapsedTime) + (1.0 - alpha) * accelAngleX;
  compAngleY = alpha * (compAngleY + gyroY * elapsedTime) + (1.0 - alpha) * accelAngleY;
  
  // For yaw, we use only gyroscope integration (accelerometer cannot measure yaw)
  compAngleZ += gyroZ * elapsedTime;
  
  // Convert to roll, pitch, yaw
  roll = compAngleX;
  pitch = compAngleY;
  yaw = compAngleZ;
  
  // Display results at a reasonable rate (20Hz instead of 100Hz)
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime >= 50) {  // 20Hz update rate for display
    Serial.printf("%.1f,%.1f,%.1f\n", roll, pitch, yaw);
    lastPrintTime = millis();
  }
  
  delay(10);  // Maintain 100Hz sensor sampling
}

void calibrateSensors() {
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
}
