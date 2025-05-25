/*
 * MPU6050 Visualization for RP2040
 * 
 * This sketch demonstrates I2C communication with an MPU6050 IMU sensor
 * and visualizes the orientation data using ASCII graphics in the serial monitor.
 * It's configured to use GPIO26 as SDA and GPIO27 as SCL.
 */

#include <Arduino.h>
#include <Wire.h>

// MPU6050 I2C address
#define MPU_ADDR 0x68

// I2C pins
const int SDA_PIN = 26;
const int SCL_PIN = 27;

// Variables for sensor data
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
int16_t temperature;

// Calibration offsets
float accelXoffset = 0, accelYoffset = 0, accelZoffset = 0;
float gyroXoffset = 0, gyroYoffset = 0, gyroZoffset = 0;
bool calibrated = false;

// Calculated values
float ax, ay, az; // Acceleration in g
float gx, gy, gz; // Gyro in degrees/s
float temp_c;     // Temperature in °C

// Filtered values for noise reduction
// Removed low-pass filter as per user request
// Using raw sensor values directly
float ax_filtered = 0, ay_filtered = 0, az_filtered = 0;
float gx_filtered = 0, gy_filtered = 0, gz_filtered = 0;

// Orientation angles (degrees)
float roll = 0.0;
float pitch = 0.0;

// Complementary filter coefficient
float alpha = 0.98; // Can be tuned between 0.95 and 0.99 for noise vs responsiveness

// Low-pass filter coefficient for sensor data smoothing
const float lp_alpha = 0.85; // Reduced filtering for more responsiveness

// Update interval
const long interval = 20; // milliseconds (50 Hz update rate)
unsigned long previousMillis = 0;

// Function prototypes
void initMPU6050();
bool readMPU6050Data();
void visualizeOrientation();
void drawHorizontalBar(float value, float maxValue);
void drawArrow(int direction);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(2000); // Wait for serial monitor to open
  
  Serial.println("\n\n*** MPU6050 Visualization ***");
  Serial.println("-----------------------------");
  
  // Initialize I2C communication, must be Wire1 because we're using I2C1 on RP2040
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();
  
  // Initialize MPU6050
  initMPU6050();
  
  Serial.println("\nTilt the sensor to see the visualization!");
  Serial.println("---------------------------------------");
}

void printInstructions() {
  Serial.println("\n=== INSTRUCTIONS ===");
  Serial.println("Commands:");
  Serial.println("  'c' or 'C' - Start calibration process");
  Serial.println("  'r' or 'R' - Reset calibration data");
  Serial.println("  's' or 'S' - Show current calibration values");
  Serial.println("====================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    if (readMPU6050Data()) {
      // Clear screen with ANSI escape code
      Serial.write(27);     // ESC
      Serial.print("[2J");  // Clear screen
      Serial.write(27);     // ESC
      Serial.print("[H");   // Move cursor to home position
      
      // Serial.println("MPU6050 Visualization");
      // Serial.println("====================");
      visualizeOrientation();
    }
  }
  
  // Check for serial commands
  if (Serial.available()) {
    char command = Serial.read();
    processSerialCommand(command);
  }
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
    readMPU6050Data();
    
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

void initMPU6050() {
  Serial.print("Initializing MPU6050... ");
  
  // Wake up the MPU6050
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x6B);  // PWR_MGMT_1 register
  Wire1.write(0);     // set to zero (wakes up the MPU6050)
  int status = Wire1.endTransmission(true);
  
  if (status != 0) {
    Serial.println("FAILED!");
    Serial.print("I2C error code: ");
    Serial.println(status);
    Serial.println("Check connections and restart.");
    while (1) {
      // Blink LED to indicate error
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  // Set I2C clock to 100kHz for stability
  Wire1.setClock(100000);
  
  // Configure Accelerometer to ±4g for less noise
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x1C);  // ACCEL_CONFIG register
  Wire1.write(0x08);  // ±4g range (01 << 3)
  Wire1.endTransmission(true);
  
  // Configure Gyroscope to ±500 deg/s for less noise
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x1B);  // GYRO_CONFIG register
  Wire1.write(0x08);  // ±500 deg/s range (01 << 3)
  Wire1.endTransmission(true);
  
  // Configure Digital Low Pass Filter to 44 Hz for noise reduction
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x1A);  // CONFIG register
  Wire1.write(0x03);  // DLPF_CFG = 3 (44 Hz)
  Wire1.endTransmission(true);
  
  // Set sample rate divider to 4 (1kHz / (1 + 4) = 200 Hz)
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x19);  // SMPLRT_DIV register
  Wire1.write(0x04);
  Wire1.endTransmission(true);
  
  Serial.println("SUCCESS!");
}

bool readMPU6050Data() {
  // Request data from MPU6050
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x3B);  // Starting with register 0x3B (ACCEL_XOUT_H)
  if (Wire1.endTransmission(false) != 0) {
    Serial.println("Error: Failed to request data from MPU6050");
    return false;
  }
  
  // Request 14 registers (6 for accel, 2 for temp, 6 for gyro)
  Wire1.requestFrom(MPU_ADDR, 14, true);
  
  if (Wire1.available() < 14) {
    Serial.println("Error: Not enough data received from MPU6050");
    return false;
  }
  
  // Read accelerometer data
  accelX = Wire1.read() << 8 | Wire1.read();
  accelY = Wire1.read() << 8 | Wire1.read();
  accelZ = Wire1.read() << 8 | Wire1.read();
  
  // Read temperature data
  temperature = Wire1.read() << 8 | Wire1.read();
  
  // Read gyroscope data
  gyroX = Wire1.read() << 8 | Wire1.read();
  gyroY = Wire1.read() << 8 | Wire1.read();
  gyroZ = Wire1.read() << 8 | Wire1.read();
  
  // Convert raw values to meaningful units
  ax = accelX / 16384.0;  // convert to g (±2g scale)
  ay = accelY / 16384.0;
  az = accelZ / 16384.0;
  
  gx = gyroX / 131.0;     // convert to degrees per second (±250°/s scale)
  gy = gyroY / 131.0;
  gz = gyroZ / 131.0;
  
  temp_c = temperature / 340.0 + 36.53;  // convert to Celsius
  
  return true;
}


#include <ArduinoJson.h>

void visualizeOrientation() {
  // Prepare JSON object
  StaticJsonDocument<256> doc;
  
  // Apply low-pass filter to smooth sensor data
  // Removed low-pass filter as per user request
  ax_filtered = ax;
  ay_filtered = ay;
  az_filtered = az;
  
  gx_filtered = gx;
  gy_filtered = gy;
  gz_filtered = gz;
  
  // Calculate calibrated values
  float accelXcal = ax_filtered - accelXoffset;
  float accelYcal = ay_filtered - accelYoffset;
  float accelZcal = az_filtered - accelZoffset;
  
  float gyroXcal = gx_filtered - gyroXoffset;
  float gyroYcal = gy_filtered - gyroYoffset;
  float gyroZcal = gz_filtered - gyroZoffset;
  
  // Debug: print raw and calibrated accelerometer values
  Serial.print("Raw Accel (g): X=");
  Serial.print(ax_filtered, 3);
  Serial.print(" Y=");
  Serial.print(ay_filtered, 3);
  Serial.print(" Z=");
  Serial.println(az_filtered, 3);
  
  Serial.print("Calib Accel (g): X=");
  Serial.print(accelXcal, 3);
  Serial.print(" Y=");
  Serial.print(accelYcal, 3);
  Serial.print(" Z=");
  Serial.println(accelZcal, 3);
  
  // Calculate roll and pitch from accelerometer (in degrees)
  // Adjust axis mapping and signs if needed to match physical orientation
  float rollAcc = atan2(accelYcal, accelZcal) * 180.0 / PI;
  float pitchAcc = atan2(-accelXcal, sqrt(accelYcal * accelYcal + accelZcal * accelZcal)) * 180.0 / PI;
  
  // Integrate gyroscope data (degrees per second) over time
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;
  
  // Complementary filter to combine accelerometer and gyro
  roll = alpha * (roll + gyroXcal * dt) + (1 - alpha) * rollAcc;
  pitch = alpha * (pitch + gyroYcal * dt) + (1 - alpha) * pitchAcc;
  
  // Fill JSON object
  doc["temperature"] = temp_c;
  JsonArray accel = doc.createNestedArray("accel");
  accel.add(accelXcal);
  accel.add(accelYcal);
  accel.add(accelZcal);
  JsonArray gyro = doc.createNestedArray("gyro");
  gyro.add(gyroXcal);
  gyro.add(gyroYcal);
  gyro.add(gyroZcal);
  JsonArray orientation = doc.createNestedArray("orientation");
  orientation.add(roll);
  orientation.add(pitch);
  doc["motion_detected"] = (abs(gyroXcal) > 15 || abs(gyroYcal) > 15 || abs(gyroZcal) > 15);
  
  // Serialize JSON to string and send over serial
  serializeJson(doc, Serial);
  Serial.println();
}

void drawHorizontalBar(float value, float maxValue) {
  // Constrain value between -maxValue and maxValue
  float constrainedValue = constrain(value, -maxValue, maxValue);
  
  // Scale to -10 to 10 range for display
  int barLength = (int)(constrainedValue * 10 / maxValue);
  
  // Draw negative side
  Serial.print("[-");
  for (int i = -10; i < 0; i++) {
    if (i >= barLength) Serial.print("#");
    else Serial.print(" ");
  }
  
  // Draw center
  Serial.print("|");
  
  // Draw positive side
  for (int i = 0; i < 10; i++) {
    if (i < barLength) Serial.print("#");
    else Serial.print(" ");
  }
  Serial.print("-]");
  
  // Print numeric value
  Serial.print("  ");
  Serial.print(value, 2);
}
