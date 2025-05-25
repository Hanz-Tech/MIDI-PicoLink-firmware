#include <Arduino.h>
#include <Wire.h>

// MPU6050 I2C address
#define MPU6050_ADDR 0x68

// MPU6050 register addresses
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_WHO_AM_I     0x75

// I2C pins
#define SDA_PIN 26
#define SCL_PIN 27

// Variables for storing sensor data
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
int16_t temperature;

// Variables for calculations
float accelXg, accelYg, accelZg;
float gyroXdeg, gyroYdeg, gyroZdeg;
float tempC;

// Timer for regular readings
unsigned long lastReadTime = 0;
const unsigned long readInterval = 100; // 100ms = 10 readings per second

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("MPU6050 Test");

  // Initialize I2C with custom pins
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();
  
  // Wake up the MPU6050
  Wire1.beginTransmission(MPU6050_ADDR);
  Wire1.write(MPU6050_PWR_MGMT_1);
  Wire1.write(0);  // Set to zero to wake up
  Wire1.endTransmission(true);
  
  // Check if the MPU6050 is responding
  Wire1.beginTransmission(MPU6050_ADDR);
  Wire1.write(MPU6050_WHO_AM_I);
  Wire1.endTransmission(false);
  Wire1.requestFrom(MPU6050_ADDR, 1, true);
  
  if (Wire1.available()) {
    byte whoAmI = Wire1.read();
    Serial.print("WHO_AM_I register value: 0x");
    Serial.println(whoAmI, HEX);
    
    if (whoAmI == 0x68 || whoAmI == 0x72) {
      Serial.println("MPU6050 detected successfully!");
    } else {
      Serial.println("Error: MPU6050 not detected. Check your wiring!");
      while (1) {
        // Halt if sensor not found
        delay(100);
      }
    }
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensor data at the specified interval
  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;
    readSensorData();
    calculateValues();
    printData();
  }
}

void readSensorData() {
  // Start reading acceleration data
  Wire1.beginTransmission(MPU6050_ADDR);
  Wire1.write(MPU6050_ACCEL_XOUT_H);  // Starting with register 0x3B (ACCEL_XOUT_H)
  Wire1.endTransmission(false);
  Wire1.requestFrom(MPU6050_ADDR, 14, true);  // Request 14 bytes
  
  // Read 14 bytes of data
  // Accelerometer: 3 axes x 2 bytes = 6 bytes
  // Temperature: 2 bytes
  // Gyroscope: 3 axes x 2 bytes = 6 bytes
  if (Wire1.available() == 14) {
    accelX = Wire1.read() << 8 | Wire1.read();  // X-axis acceleration
    accelY = Wire1.read() << 8 | Wire1.read();  // Y-axis acceleration
    accelZ = Wire1.read() << 8 | Wire1.read();  // Z-axis acceleration
    
    temperature = Wire1.read() << 8 | Wire1.read();  // Temperature
    
    gyroX = Wire1.read() << 8 | Wire1.read();  // X-axis rotation
    gyroY = Wire1.read() << 8 | Wire1.read();  // Y-axis rotation
    gyroZ = Wire1.read() << 8 | Wire1.read();  // Z-axis rotation
  }
}

void calculateValues() {
  // Convert raw acceleration values to g forces
  // Accelerometer sensitivity is ±2g by default
  // So raw values are divided by 16384 according to datasheet
  accelXg = accelX / 16384.0;
  accelYg = accelY / 16384.0;
  accelZg = accelZ / 16384.0;
  
  // Convert raw gyro values to degrees per second
  // Gyro sensitivity is ±250 degrees/second by default
  // So raw values are divided by 131 according to datasheet
  gyroXdeg = gyroX / 131.0;
  gyroYdeg = gyroY / 131.0;
  gyroZdeg = gyroZ / 131.0;
  
  // Convert raw temperature to celsius
  // Formula from datasheet: Temp(°C) = (TEMP_OUT/340) + 36.53
  tempC = (temperature / 340.0) + 36.53;
}

void printData() {
  // Print formatted data
  Serial.println("----------------------------------------------------------");
  Serial.println("MPU6050 Sensor Data:");
  
  // Accelerometer data (g forces)
  Serial.print("Acceleration (g) | X: ");
  Serial.print(accelXg, 2);  // 2 decimal places
  Serial.print("  Y: ");
  Serial.print(accelYg, 2);
  Serial.print("  Z: ");
  Serial.println(accelZg, 2);
  
  // Gyroscope data (degrees per second)
  Serial.print("Gyroscope (°/s)  | X: ");
  Serial.print(gyroXdeg, 2);
  Serial.print("  Y: ");
  Serial.print(gyroYdeg, 2);
  Serial.print("  Z: ");
  Serial.println(gyroZdeg, 2);
  
  // Temperature data
  Serial.print("Temperature      | ");
  Serial.print(tempC, 1);  // 1 decimal place
  Serial.println(" °C");
  
  // Calculate approximate orientation based on accelerometer
  if (abs(accelXg) > abs(accelYg) && abs(accelXg) > abs(accelZg)) {
    if (accelXg > 0) {
      Serial.println("Orientation      | Tilted RIGHT");
    } else {
      Serial.println("Orientation      | Tilted LEFT");
    }
  } else if (abs(accelYg) > abs(accelXg) && abs(accelYg) > abs(accelZg)) {
    if (accelYg > 0) {
      Serial.println("Orientation      | Tilted BACK");
    } else {
      Serial.println("Orientation      | Tilted FORWARD");
    }
  } else {
    if (accelZg > 0) {
      Serial.println("Orientation      | UPSIDE DOWN");
    } else {
      Serial.println("Orientation      | NORMAL");
    }
  }
}
