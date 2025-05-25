/*
 * MPU6050 Basic Test with Calibration for RP2040
 * 
 * This sketch uses direct Wire library calls with a simplified calibration
 * algorithm based on FastIMU's approach. It's configured to use GPIO26 as SDA and GPIO27 as SCL.
 * 
 * Features:
 * - Direct Wire I2C communication (proven to work)
 * - Simple but effective calibration algorithm
 * - Calibrated sensor output
 * 
 * Commands:
 * - Send 'c' or 'C' via Serial to start calibration
 * - Send 'r' or 'R' to reset calibration values
 */

#include <Arduino.h>
#include <Wire.h>

// MPU6050 I2C address
#define MPU_ADDR 0x68

// I2C pins for the RP2040
const int SDA_PIN = 26;
const int SCL_PIN = 27;

// Variables to store sensor readings
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
int16_t temperature;

// Calibration variables
float gyroXoffset = 0, gyroYoffset = 0, gyroZoffset = 0;
float accelXoffset = 0, accelYoffset = 0, accelZoffset = 0;
bool calibrated = false;

// Update interval
const long interval = 500; // milliseconds
unsigned long previousMillis = 0;

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\nMPU6050 with Wire Library + Calibration");
  Serial.println("=======================================");
  Serial.print("Configuring I2C: SDA on GPIO");
  Serial.print(SDA_PIN);
  Serial.print(", SCL on GPIO");
  Serial.println(SCL_PIN);
  
  // Initialize I2C with custom pins
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  Wire.setClock(400000); // 400kHz clock
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  if (!initMPU6050()) {
    Serial.println("Failed to initialize MPU6050!");
    blinkError();
  }
  
  Serial.println("MPU6050 initialized successfully!");
  printInstructions();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char command = Serial.read();
    processSerialCommand(command);
  }
  
  unsigned long currentMillis = millis();
  
  // Read and display sensor data at regular intervals
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    readSensorData();
    printData();
  }
}

bool initMPU6050() {
  // Check WHO_AM_I register
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75); // WHO_AM_I register
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 1, true);
  
  if (Wire.available()) {
    byte whoAmI = Wire.read();
    Serial.print("WHO_AM_I: 0x");
    Serial.println(whoAmI, HEX);
    
    if (whoAmI != 0x68 && whoAmI != 0x72) {
      Serial.println("Error: Unknown device detected");
      return false;
    }
  } else {
    Serial.println("Error: Cannot read WHO_AM_I register");
    return false;
  }
  
  // Wake up the MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // Wake up
  if (Wire.endTransmission(true) != 0) {
    Serial.println("Error: Failed to wake up MPU6050");
    return false;
  }
  
  delay(100);
  
  // Set accelerometer range to ±2g
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); // ACCEL_CONFIG register
  Wire.write(0x00); // ±2g range
  Wire.endTransmission(true);
  
  // Set gyroscope range to ±250°/s
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); // GYRO_CONFIG register
  Wire.write(0x00); // ±250°/s range
  Wire.endTransmission(true);
  
  return true;
}

void readSensorData() {
  // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  
  // Request 14 registers (6 for accel, 2 for temp, 6 for gyro)
  Wire.requestFrom(MPU_ADDR, 14, true);
  
  if (Wire.available() == 14) {
    accelX = (Wire.read() << 8) | Wire.read();
    accelY = (Wire.read() << 8) | Wire.read();
    accelZ = (Wire.read() << 8) | Wire.read();
    temperature = (Wire.read() << 8) | Wire.read();
    gyroX = (Wire.read() << 8) | Wire.read();
    gyroY = (Wire.read() << 8) | Wire.read();
    gyroZ = (Wire.read() << 8) | Wire.read();
  }
}

void printData() {
  Serial.println("\n--- MPU6050 Sensor Data ---");
  
  // Calculate raw values in meaningful units
  float accelXg = accelX / 16384.0; // ±2g scale
  float accelYg = accelY / 16384.0;
  float accelZg = accelZ / 16384.0;
  
  float gyroXdps = gyroX / 131.0; // ±250°/s scale
  float gyroYdps = gyroY / 131.0;
  float gyroZdps = gyroZ / 131.0;
  
  float tempC = (temperature / 340.0) + 36.53;
  
  // Print raw values
  Serial.print("Accel (g):   X=");
  Serial.print(accelXg, 2);
  Serial.print(", Y=");
  Serial.print(accelYg, 2);
  Serial.print(", Z=");
  Serial.println(accelZg, 2);
  
  Serial.print("Gyro (°/s):  X=");
  Serial.print(gyroXdps, 1);
  Serial.print(", Y=");
  Serial.print(gyroYdps, 1);
  Serial.print(", Z=");
  Serial.println(gyroZdps, 1);
  
  Serial.print("Temp:        ");
  Serial.print(tempC, 1);
  Serial.println(" °C");
  
  // Print calibrated values if available
  if (calibrated) {
    Serial.println("\n--- Calibrated Data ---");
    
    // Apply calibration offsets
    float accelXcal = accelXg - accelXoffset;
    float accelYcal = accelYg - accelYoffset;
    float accelZcal = accelZg - accelZoffset;
    
    float gyroXcal = gyroXdps - gyroXoffset;
    float gyroYcal = gyroYdps - gyroYoffset;
    float gyroZcal = gyroZdps - gyroZoffset;
    
    Serial.print("Accel Cal(g): X=");
    Serial.print(accelXcal, 2);
    Serial.print(", Y=");
    Serial.print(accelYcal, 2);
    Serial.print(", Z=");
    Serial.println(accelZcal, 2);
    
    Serial.print("Gyro Cal(°/s): X=");
    Serial.print(gyroXcal, 1);
    Serial.print(", Y=");
    Serial.print(gyroYcal, 1);
    Serial.print(", Z=");
    Serial.println(gyroZcal, 1);
  } else {
    Serial.println("(Data is NOT calibrated - send 'c' to calibrate)");
  }
}

void printInstructions() {
  Serial.println("\n=== INSTRUCTIONS ===");
  Serial.println("Commands:");
  Serial.println("  'c' or 'C' - Start calibration process");
  Serial.println("  'r' or 'R' - Reset calibration data");
  Serial.println("  's' or 'S' - Show current calibration values");
  Serial.println("====================");
}

void processSerialCommand(char command) {
  // Clear the serial buffer
  while (Serial.available()) {
    Serial.read();
  }
  
  switch (command) {
    case 'c':
    case 'C':
      performCalibration();
      break;
      
    case 'r':
    case 'R':
      resetCalibration();
      break;
      
    case 's':
    case 'S':
      showCalibrationValues();
      break;
      
    default:
      Serial.println("\nUnknown command.");
      printInstructions();
      break;
  }
}

void performCalibration() {
  Serial.println("\n=== STARTING CALIBRATION ===");
  Serial.println("Keep IMU level and stationary.");
  Serial.println("Calibration will start in 5 seconds...");
  delay(5000);
  
  Serial.print("Calibrating");
  
  const int numSamples = 1000;
  long accelXsum = 0, accelYsum = 0, accelZsum = 0;
  long gyroXsum = 0, gyroYsum = 0, gyroZsum = 0;
  
  // Collect samples for averaging
  for (int i = 0; i < numSamples; i++) {
    readSensorData();
    
    accelXsum += accelX;
    accelYsum += accelY;
    accelZsum += accelZ;
    gyroXsum += gyroX;
    gyroYsum += gyroY;
    gyroZsum += gyroZ;
    
    if (i % 100 == 0) {
      Serial.print(".");
    }
    delay(5);
  }
  Serial.println();
  
  // Calculate offsets (in g and °/s)
  accelXoffset = (float)accelXsum / numSamples / 16384.0;
  accelYoffset = (float)accelYsum / numSamples / 16384.0;
  accelZoffset = ((float)accelZsum / numSamples / 16384.0) - 1.0; // Z should be 1g when level
  
  gyroXoffset = (float)gyroXsum / numSamples / 131.0;
  gyroYoffset = (float)gyroYsum / numSamples / 131.0;
  gyroZoffset = (float)gyroZsum / numSamples / 131.0;
  
  calibrated = true;
  
  Serial.println("Calibration completed!");
  showCalibrationValues();
}

void resetCalibration() {
  Serial.println("\nResetting calibration data...");
  
  gyroXoffset = 0;
  gyroYoffset = 0;
  gyroZoffset = 0;
  accelXoffset = 0;
  accelYoffset = 0;
  accelZoffset = 0;
  calibrated = false;
  
  Serial.println("Calibration data reset!");
}

void showCalibrationValues() {
  Serial.println("\n=== CALIBRATION VALUES ===");
  
  if (calibrated) {
    Serial.println("Accelerometer offsets (X/Y/Z):");
    Serial.print("  ");
    Serial.print(accelXoffset, 4);
    Serial.print(", ");
    Serial.print(accelYoffset, 4);
    Serial.print(", ");
    Serial.println(accelZoffset, 4);
    
    Serial.println("Gyroscope offsets (X/Y/Z):");
    Serial.print("  ");
    Serial.print(gyroXoffset, 4);
    Serial.print(", ");
    Serial.print(gyroYoffset, 4);
    Serial.print(", ");
    Serial.println(gyroZoffset, 4);
  } else {
    Serial.println("No calibration data available.");
    Serial.println("Send 'c' to calibrate the sensor.");
  }
  Serial.println("========================");
}

void blinkError() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("ERROR: Check connections and restart.");
  
  while (1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}
