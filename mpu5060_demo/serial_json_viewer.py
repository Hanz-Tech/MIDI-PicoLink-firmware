import serial
import json
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Configure your serial port and baud rate
SERIAL_PORT = 'COM21'  # Change as needed
BAUD_RATE = 115200

# Data buffers
max_len = 100
time_data = deque(maxlen=max_len)
accel_x = deque(maxlen=max_len)
accel_y = deque(maxlen=max_len)
accel_z = deque(maxlen=max_len)
gyro_x = deque(maxlen=max_len)
gyro_y = deque(maxlen=max_len)
gyro_z = deque(maxlen=max_len)
roll_data = deque(maxlen=max_len)
pitch_data = deque(maxlen=max_len)
motion_data = deque(maxlen=max_len)

def read_serial_data(ser):
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line:
            continue
        try:
            data = json.loads(line)
            yield data
        except json.JSONDecodeError:
            continue

def update_plot(frame, lines, data_gen):
    try:
        data = next(data_gen)
        time_data.append(frame)
        accel_x.append(data['accel'][0])
        accel_y.append(data['accel'][1])
        accel_z.append(data['accel'][2])
        gyro_x.append(data['gyro'][0])
        gyro_y.append(data['gyro'][1])
        gyro_z.append(data['gyro'][2])
        roll_data.append(data['orientation'][0])
        pitch_data.append(data['orientation'][1])
        motion_data.append(1 if data['motion_detected'] else 0)

        lines[0].set_data(time_data, accel_x)
        lines[1].set_data(time_data, accel_y)
        lines[2].set_data(time_data, accel_z)
        lines[3].set_data(time_data, gyro_x)
        lines[4].set_data(time_data, gyro_y)
        lines[5].set_data(time_data, gyro_z)
        lines[6].set_data(time_data, roll_data)
        lines[7].set_data(time_data, pitch_data)
        lines[8].set_data(time_data, motion_data)
        
        for ax in lines[0].axes.figure.axes:
            ax.relim()
            ax.autoscale(enable=True)
    except StopIteration:
        pass
    return lines

def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    data_gen = read_serial_data(ser)

    fig, axs = plt.subplots(4, 1, figsize=(10, 12))
    axs[0].set_title('Accelerometer (g)')
    axs[0].set_xlim(0, max_len)
    axs[0].set_ylim(-2, 2)
    axs[0].set_ylabel('g')
    axs[0].grid(True)
    line_ax, = axs[0].plot([], [], label='Accel X')
    line_ay, = axs[0].plot([], [], label='Accel Y')
    line_az, = axs[0].plot([], [], label='Accel Z')
    axs[0].legend(loc='upper right')

    axs[1].set_title('Gyroscope (°/s)')
    axs[1].set_xlim(0, max_len)
    axs[1].set_ylim(-250, 250)
    axs[1].set_ylabel('°/s')
    axs[1].grid(True)
    line_gx, = axs[1].plot([], [], label='Gyro X')
    line_gy, = axs[1].plot([], [], label='Gyro Y')
    line_gz, = axs[1].plot([], [], label='Gyro Z')
    axs[1].legend(loc='upper right')

    axs[2].set_title('Orientation (degrees)')
    axs[2].set_xlim(0, max_len)
    axs[2].set_ylim(-180, 180)
    axs[2].set_ylabel('Degrees')
    axs[2].grid(True)
    line_roll, = axs[2].plot([], [], label='Roll')
    line_pitch, = axs[2].plot([], [], label='Pitch')
    axs[2].legend(loc='upper right')

    axs[3].set_title('Motion Detection')
    axs[3].set_xlim(0, max_len)
    axs[3].set_ylim(-0.1, 1.1)
    axs[3].set_ylabel('Motion')
    axs[3].set_xlabel('Samples')
    axs[3].grid(True)
    line_motion, = axs[3].plot([], [], label='Motion Detected')
    axs[3].legend(loc='upper right')

    lines = [line_ax, line_ay, line_az, line_gx, line_gy, line_gz, line_roll, line_pitch, line_motion]

    ani = animation.FuncAnimation(fig, update_plot, fargs=(lines, data_gen), interval=50, blit=False)

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
