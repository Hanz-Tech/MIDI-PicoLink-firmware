#include <cstdint>  // Fix for uint8_t in pio_usb_configuration.h
#include "pio_usb_configuration.h"
#include <Arduino.h>
#include <MIDI.h>
#include "usb_host_wrapper.h"
#include "pio_usb.h"
#include "led_utils.h"
#include "midi_instances.h"
#include "midi_router.h"

// Include the new Serial MIDI header
#include "serial_midi_handler.h"
#include "midi_filters.h"
#include "serial_utils.h"
#include "web_serial_config.h"
#include "config.h"
#include "imu_handler.h"

// USB Host configuration
#define HOST_PIN_DP   12   // Pin used as D+ for host, D- = D+ + 1

// Serial MIDI pins configuration - MOVED TO serial_midi.cpp
// int serialRxPin = 1;   // GPIO pin for Serial1 RX
// int serialTxPin = 0;   // GPIO pin for Serial1 TX

// USB MIDI device address (set by onMIDIconnect callback in usb_host_wrapper.cpp)
// Make sure this is defined (not static) in usb_host_wrapper.cpp so it's linkable
extern volatile uint8_t midi_dev_addr;
extern volatile bool midi_host_mounted;

volatile bool isConnectedToComputer = false;

// USB Host object
Adafruit_USBH_Host USBHost;

// Create USB Device MIDI instance
// USB_D is now created by the macro in midi_instances.h

// Create Serial MIDI instance using Serial1 - MOVED TO serial_midi.cpp
// MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SERIAL_M);

// USB Host MIDI
USING_NAMESPACE_MIDI


volatile bool core1_booting = true;
uint32_t timeout = 2000; // 2 seconds timeout

// Forward declarations for USB Host MIDI message handlers (remain the same)
void usbh_onNoteOffHandle(byte channel, byte note, byte velocity);
void usbh_onNoteOnHandle(byte channel, byte note, byte velocity);
void usbh_onPolyphonicAftertouchHandle(byte channel, byte note, byte amount);
void usbh_onControlChangeHandle(byte channel, byte controller, byte value);
void usbh_onProgramChangeHandle(byte channel, byte program);
void usbh_onAftertouchHandle(byte channel, byte value);
void usbh_onPitchBendHandle(byte channel, int value);
void usbh_onSysExHandle(byte * array, unsigned size);
void usbh_onMidiClockHandle();
void usbh_onMidiStartHandle();
void usbh_onMidiContinueHandle();
void usbh_onMidiStopHandle();

// C-style wrappers for usb_host_wrapper.cpp linkage
void onNoteOff(Channel channel, byte note, byte velocity) { usbh_onNoteOffHandle(channel, note, velocity); }
void onNoteOn(Channel channel, byte note, byte velocity) { usbh_onNoteOnHandle(channel, note, velocity); }
void onPolyphonicAftertouch(Channel channel, byte note, byte pressure) { usbh_onPolyphonicAftertouchHandle(channel, note, pressure); }
void onControlChange(Channel channel, byte control, byte value) { usbh_onControlChangeHandle(channel, control, value); }
void onProgramChange(Channel channel, byte program) { usbh_onProgramChangeHandle(channel, program); }
void onAftertouch(Channel channel, byte pressure) { usbh_onAftertouchHandle(channel, pressure); }
void onPitchBend(Channel channel, int bend) { usbh_onPitchBendHandle(channel, bend); }
void onSysEx(byte * array, unsigned size) { usbh_onSysExHandle(array, size); }
void onMidiClock() { usbh_onMidiClockHandle(); }
void onMidiStart() { usbh_onMidiStartHandle(); }
void onMidiContinue() { usbh_onMidiContinueHandle(); }
void onMidiStop() { usbh_onMidiStopHandle(); }

// USB Device MIDI message handlers (remain the same)
void usbd_onNoteOn(byte channel, byte note, byte velocity);
void usbd_onNoteOff(byte channel, byte note, byte velocity);
void usbd_onControlChange(byte channel, byte controller, byte value);
void usbd_onProgramChange(byte channel, byte program);
void usbd_onAftertouch(byte channel, byte pressure);
void usbd_onPitchBend(byte channel, int bend);
void usbd_onSysEx(byte * array, unsigned size);
void usbd_onClock();
void usbd_onStart();
void usbd_onContinue();
void usbd_onStop();

// Serial MIDI message handlers - REMOVED (defined in serial_midi.cpp)
// void serial_onNoteOn(byte channel, byte note, byte velocity);
// ... (remove all serial_on... declarations)

void setup() {
  // Set USB device descriptors BEFORE any other initialization
  // This must be done before any TinyUSB or USB MIDI calls
  TinyUSBDevice.setID(0x239A, 0x8122);  // Use Adafruit's official VID/PID for MIDI
  TinyUSBDevice.setManufacturerDescriptor("HanzTech");
  TinyUSBDevice.setProductDescriptor("MIDI PicoLink");
  TinyUSBDevice.setSerialDescriptor("PicoLink-001");
  
  // Initialize debug serial early
  Serial.begin(115200);
  Serial2.setRX(25);
  Serial2.setTX(24);
  Serial2.begin(115200);

  dualPrintln("DEBUG: Entered setup()");
  Serial2.println("DEBUG: Core0 start Serial2");
  // Configure LED pins
  pinMode(LED_IN_PIN, OUTPUT);
  pinMode(LED_OUT_PIN, OUTPUT);
  digitalWrite(LED_IN_PIN, LOW);
  digitalWrite(LED_OUT_PIN, LOW);
  initLEDs();

  // Configure Serial1 pins - MOVED TO setupSerialMidi()
  // Serial1.setRX(serialRxPin);
  // Serial1.setTX(serialTxPin);

  // Initialize USB MIDI device - descriptors set at top of setup()
  usb_midi.setStringDescriptor("MIDI PicoLink");
  usb_midi.begin();

  // Initialize USB MIDI device handlers
  USB_D.begin(MIDI_CHANNEL_OMNI);
  USB_D.setHandleNoteOn(usbd_onNoteOn);
  USB_D.setHandleNoteOff(usbd_onNoteOff);
  USB_D.setHandleControlChange(usbd_onControlChange);
  USB_D.setHandleProgramChange(usbd_onProgramChange);
  USB_D.setHandleAfterTouchChannel(usbd_onAftertouch);
  USB_D.setHandlePitchBend(usbd_onPitchBend);
  USB_D.setHandleSystemExclusive(usbd_onSysEx);
  USB_D.setHandleClock(usbd_onClock);
  USB_D.setHandleStart(usbd_onStart);
  USB_D.setHandleContinue(usbd_onContinue);
  USB_D.setHandleStop(usbd_onStop);

  // Initialize Serial MIDI - MOVED TO setupSerialMidi()

  // Add a timeout for USB device mounting (2 seconds max wait)
  uint32_t startTime = millis();
  
  // Wait for device to be mounted, but with a timeout
  while(!TinyUSBDevice.mounted()) {
    delay(1);
    // Break the loop if timeout is reached
    if(millis() - startTime > timeout) {
      dualPrintln("USB device not mounted after timeout - likely connected to power bank");
      break;
    }
  }
  
  // Check if we're connected to a computer or just a power source
  isConnectedToComputer = TinyUSBDevice.mounted();
  
  dualPrintln("RP2040 USB MIDI Router - Main Sketch");
  if(isConnectedToComputer) {
    dualPrintln("Connected to computer - USB Device mode active");
  } else {
    dualPrintln("Connected to power source only - Running in standalone mode");
  }

  // Initialize MIDI filters
  setupMidiFilters();
  enableAllChannels();
  // Load persisted filter/channel config from EEPROM
  loadConfigFromEEPROM();
  
  // Initialize IMU if enabled
  if (setupIMU()) {
    dualPrintln("IMU initialized successfully");
    // Uncomment to auto-calibrate on startup
    // calibrateIMU();
  } else {
    dualPrintln("IMU initialization failed or not enabled");
  }
  

  // Call the setup function for the Serial MIDI module
  setupSerialMidi();

  rp2040.fifo.push(0);
  while(rp2040.fifo.pop() != 1){};
  USB_D.turnThruOff();
  dualPrintln("Core0 setup complete");
  dualPrintln("");
  blinkBothLEDs(4, 100);
}


// Main loop for core 0
void loop() {
  // Note: USB Host task now runs on core1 in loop1()
  
  // Process USB Device MIDI only if connected to a computer
  if (isConnectedToComputer) {
    USB_D.read();
  }

  // Process Serial MIDI
  loopSerialMidi(); 

  // Handle Web Serial config commands (JSON over USB CDC)
  processWebSerialConfig();

  // Handle delayed EEPROM saves (non-blocking)
  handleDelayedEEPROMSave();

  // Process IMU and send MIDI CC messages
  loopIMU();

  // Handle LED indicators
  handleLEDs();
}

void setup1() {
  while(rp2040.fifo.pop() != 0){};
  if (!isConnectedToComputer) {
    // We're in standalone mode, don't wait for Serial
    dualPrintln("Core1 setup in standalone mode");
  } else {
    // Only wait briefly for Serial when connected to computer
    uint32_t startTime = millis();
    while(!Serial && (millis() - startTime < timeout)); // 2 second timeout
  }
  
  

  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = clock_get_hz(clk_sys);
  if (cpu_hz != 120000000UL && cpu_hz != 240000000UL) {
    delay(2000);   // wait for native usb
    dualPrintf("Error: CPU Clock = %u, PIO USB require CPU clock must be multiple of 120 Mhz\r\n", cpu_hz);
    dualPrintf("Change your CPU Clock to either 120 or 240 Mhz in Menu->CPU Speed\r\n");
    while(1) delay(1);
  }

  // Configure PIO USB with proper structure
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = HOST_PIN_DP;

  #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    /* Need to swap PIOs so PIO code from CYW43 PIO SPI driver will fit */
    pio_cfg.pio_rx_num = 0;
    pio_cfg.pio_tx_num = 1;
  #endif

  dualPrintf("Core1: Configuring PIO USB with DP pin %u\r\n", HOST_PIN_DP);
  USBHost.configure_pio_usb(1, &pio_cfg);

  // Initialize USB Host - TinyUSB will automatically handle MIDI devices
  dualPrintf("Core1: Starting USB Host...\r\n");
  bool host_init_success = USBHost.begin(1);
  if (host_init_success) {
    dualPrintf("Core1: USB Host initialized successfully\r\n");
  } else {
    dualPrintf("Core1: USB Host initialization FAILED!\r\n");
  }
  
  // Add a small delay to let USB host settle
  delay(100);
  
  rp2040.fifo.push(1);
  dualPrintln("Core1 setup to run TinyUSB host with pio-usb");
  dualPrintln("");
}

// Core1's loop - CRITICAL: This was missing!
void loop1() {
  // Call our USB host wrapper task which includes polling and USBHost.task()
  usb_host_wrapper_task();
}

// Shared variables for LED state - Remains the same
// static uint32_t inLedStartMs = 0; // Assuming these are handled in led_utils.cpp/h now
// static bool inLedActive = false;
// static uint32_t outLedStartMs = 0;
// static bool outLedActive = false;

// --- USB Host MIDI message handlers ---
// These forward messages to USB Device MIDI and Serial MIDI
// MODIFIED to use sendSerialMidi... functions

void usbh_onNoteOffHandle(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 1;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onNoteOnHandle(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 0;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onPolyphonicAftertouchHandle(byte channel, byte note, byte amount) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_POLY_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = amount;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onControlChangeHandle(byte channel, byte controller, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CONTROL_CHANGE;
  msg.channel = channel;
  msg.data1 = controller;
  msg.data2 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onProgramChangeHandle(byte channel, byte program) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PROGRAM_CHANGE;
  msg.channel = channel;
  msg.data1 = program;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onAftertouchHandle(byte channel, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CHANNEL_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onPitchBendHandle(byte channel, int value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PITCH_BEND;
  msg.channel = channel;
  msg.pitchBend = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onSysExHandle(byte * array, unsigned size) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_SYSEX;
  msg.channel = 0;
  msg.sysexData = array;
  msg.sysexSize = size;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiClockHandle() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Clock;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiStartHandle() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Start;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiContinueHandle() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Continue;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiStopHandle() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Stop;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

// --- USB Device MIDI message handlers ---
// These forward messages to USB Host MIDI and Serial MIDI
// MODIFIED to use sendSerialMidi... functions

void usbd_onNoteOn(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 0;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onNoteOff(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 1;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onControlChange(byte channel, byte controller, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CONTROL_CHANGE;
  msg.channel = channel;
  msg.data1 = controller;
  msg.data2 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onProgramChange(byte channel, byte program) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PROGRAM_CHANGE;
  msg.channel = channel;
  msg.data1 = program;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onAftertouch(byte channel, byte pressure) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CHANNEL_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = pressure;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onPitchBend(byte channel, int bend) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PITCH_BEND;
  msg.channel = channel;
  msg.pitchBend = bend;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onSysEx(byte * array, unsigned size) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_SYSEX;
  msg.channel = 0;
  msg.sysexData = array;
  msg.sysexSize = size;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

// USB Device MIDI real-time message handlers
void usbd_onClock() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Clock;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onStart() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Start;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onContinue() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Continue;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void usbd_onStop() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Stop;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}
