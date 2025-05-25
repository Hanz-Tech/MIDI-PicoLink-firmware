# MPU6050 IMU Sensor Demo

This is a demonstration program for interfacing with the MPU6050 IMU (Inertial Measurement Unit) sensor using a Raspberry Pi Pico (RP2040) microcontroller.

## Hardware Setup

1. **Connections:**
   - Connect MPU6050 SDA to GPIO26 on the RP2040
   - Connect MPU6050 SCL to GPIO27 on the RP2040
   - Connect MPU6050 VCC to 3.3V on the RP2040
   - Connect MPU6050 GND to GND on the RP2040

2. **Wiring Diagram:**
   ```
   RP2040                MPU6050
   ------                -------
   GPIO26 (SDA) -------- SDA
   GPIO27 (SCL) -------- SCL
   3.3V       -------- VCC
   GND        -------- GND
   ```

## Software Features

The demo program demonstrates:

1. **I2C Communication:**
   - Using the Wire library with custom SDA/SCL pins (26 and 27)

2. **Sensor Data Reading:**
   - Accelerometer (X, Y, Z) - in g forces
   - Gyroscope (X, Y, Z) - in degrees per second
   - Temperature - in degrees Celsius

3. **Orientation Detection:**
   - Simple algorithm to detect the orientation of the sensor based on accelerometer data

## How to Use

1. Upload the sketch `mpu6050_demo.ino` to your RP2040 board using the Arduino IDE

2. Open the Serial Monitor at 115200 baud rate

3. You should see data output every 100ms with the following information:
   - Accelerometer readings in g forces
   - Gyroscope readings in degrees per second
   - Temperature in degrees Celsius
   - Approximate orientation of the sensor

## Troubleshooting

1. **Sensor Not Detected:**
   - The program will halt if the sensor is not detected
   - Verify your wiring connections
   - Make sure the MPU6050 is powered correctly (3.3V)
   - Check for any short circuits or loose connections

2. **Strange Readings:**
   - Ensure the sensor is not exposed to strong magnetic fields
   - Keep the sensor stationary during initialization for best results
   - Try power cycling the sensor if values seem incorrect

## Customization

You can modify the following parameters in the code:

- `readInterval` - Change the data reading frequency (default: 100ms)
- `SDA_PIN` and `SCL_PIN` - Change I2C pins if needed
- Uncomment additional calculations or features in the code for more advanced usage

## Reference

This code interacts directly with the MPU6050 registers:
- `0x6B` (PWR_MGMT_1) - Power management
- `0x3B` (ACCEL_XOUT_H) - Start of accelerometer data
- `0x43` (GYRO_XOUT_H) - Start of gyroscope data
- `0x75` (WHO_AM_I) - Device identification register

For more details, refer to the MPU6050 datasheet.
