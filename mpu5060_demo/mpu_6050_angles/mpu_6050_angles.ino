/*
 * MPU6050 Roll, Pitch, and Yaw Angle Calculation
 * Based on the Adafruit MPU6050 unified sensors example
 * 
 * This sketch calculates and displays the roll, pitch, and yaw angles
 * from MPU6050 accelerometer and gyroscope data.
 */

#include <Adafruit_MPU6050.h>
#include <Wire.h>
#include <math.h>

Adafruit_MPU6050 mpu;
Adafruit_Sensor *mpu_accel, *mpu_gyro;

  // Variables for angle calculation
float roll = 0.0, pitch = 0.0, yaw = 0.0;
float gyroXangle = 0.0, gyroYangle = 0.0, gyroZangle = 0.0;
float compAngleX = 0.0, compAngleY = 0.0, compAngleZ = 0.0;
  
  // Sensitivity multiplier for yaw detection
  const float yawSensitivity = 5.0;  // Adjust based on your rotation speed

// Timing variables
unsigned long lastTime = 0;
unsigned long currentTime = 0;
float elapsedTime = 0.0;

  // Complementary filter coefficient (0.98 for gyro, 0.02 for accel)
const float alpha = 0.92;

// Offset variables for calibration
float gyroXoffset = 0.0, gyroYoffset = 0.0, gyroZoffset = 0.0;
float accelXoffset = 0.0, accelYoffset = 0.0, accelZoffset = 0.0;

// Calibration samples
const int calibrationSamples = 100;
int calibrationCounter = 0;
bool calibrated = false;

// Configure I2C pins for RP2040
// Change these to your desired SDA and SCL pins for I2C0 (Wire)
// Use I2C0 pins for Wire (primary I2C interface)
const int I2C_SDA_PIN = 4;  // Default: 4 (GP4), change as needed
const int I2C_SCL_PIN = 5;  // Default: 5 (GP5), change as needed

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("MPU6050 Angle Calculation Demo");
  Serial.println("==============================");

  Serial.println("about to set i2c");
  // Configure I2C pins for RP2040
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Serial.println("about to begin i2c");
  // Initialize I2C
  Wire.begin();
  Serial.println("i2c began");
  // Initialize MPU6050 with Wire (I2C0)
  if (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("Failed to find MPU6050 chip");
    Serial.print("I2C SDA Pin: ");
    Serial.println(I2C_SDA_PIN);
    Serial.print("I2C SCL Pin: ");
    Serial.println(I2C_SCL_PIN);
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");
  
  // Configure MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  mpu_accel = mpu.getAccelerometerSensor();
  mpu_gyro = mpu.getGyroSensor();
  
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
  sensors_event_t accel, gyro;
  mpu_accel->getEvent(&accel);
  mpu_gyro->getEvent(&gyro);
  
  // Get current time
  currentTime = millis();
  elapsedTime = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;
  
  // Apply calibration offsets
  float gyroX = gyro.gyro.x - gyroXoffset;
  float gyroY = gyro.gyro.y - gyroYoffset;
  float gyroZ = gyro.gyro.z - gyroZoffset;
  
  float accelX = accel.acceleration.x - accelXoffset;
  float accelY = accel.acceleration.y - accelYoffset;
  float accelZ = accel.acceleration.z - accelZoffset;
  
  // Calculate angles from accelerometer
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
  // Apply gyroscope integration for yaw with proper scaling and sensitivity
  compAngleZ += gyroZ * elapsedTime * yawSensitivity;
  
  // Convert to roll, pitch, yaw
  roll = compAngleX;
  pitch = compAngleY;
  yaw = compAngleZ;
  
  // Display results at a reasonable rate (20Hz instead of 100Hz)
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime >= 10) {  // 20Hz update rate for display
    Serial.printf("%.1f,%.1f,%.1f\n", roll, pitch, yaw);
    lastPrintTime = millis();
  }
  
  delay(5);  // Maintain 100Hz sensor sampling, 20Hz display
}

void calibrateSensors() {
  float gyroXsum = 0, gyroYsum = 0, gyroZsum = 0;
  float accelXsum = 0, accelYsum = 0, accelZsum = 0;
  
  for (int i = 0; i < calibrationSamples; i++) {
    sensors_event_t accel, gyro;
    mpu_accel->getEvent(&accel);
    mpu_gyro->getEvent(&gyro);
    
    gyroXsum += gyro.gyro.x;
    gyroYsum += gyro.gyro.y;
    gyroZsum += gyro.gyro.z;
    
    accelXsum += accel.acceleration.x;
    accelYsum += accel.acceleration.y;
    accelZsum += accel.acceleration.z - 9.81;  // Subtract gravity
    
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
