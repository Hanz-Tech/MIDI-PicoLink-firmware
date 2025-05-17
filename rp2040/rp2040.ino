#include <usb_midi_host.h>
#include "pio_usb_configuration.h"
#include <Arduino.h>
#include <MIDI.h>
#include "usb_host_wrapper.h"
#include "pio_usb.h"
#include "led_utils.h"
#include "midi_instances.h"

// Include the new Serial MIDI header
#include "serial_midi_handler.h"
#include "midi_filters.h"
#include "eeprom_config.h"

// USB Host configuration
#define HOST_PIN_DP   12   // Pin used as D+ for host, D- = D+ + 1
#include "EZ_USB_MIDI_HOST.h"

// Serial MIDI pins configuration - MOVED TO serial_midi.cpp
// int serialRxPin = 1;   // GPIO pin for Serial1 RX
// int serialTxPin = 0;   // GPIO pin for Serial1 TX

// USB MIDI device address (set by onMIDIconnect callback in usb_host_wrapper.cpp)
// Make sure this is defined (not static) in usb_host_wrapper.cpp so it's linkable
extern uint8_t midi_dev_addr;

// USB Host object
Adafruit_USBH_Host USBHost;

// Create USB Device MIDI instance
// USB_D is now created by the macro in midi_instances.h

// Create Serial MIDI instance using Serial1 - MOVED TO serial_midi.cpp
// MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SERIAL_M);

// USB Host MIDI
USING_NAMESPACE_MIDI
USING_NAMESPACE_EZ_USB_MIDI_HOST

// Make sure myMidiHost/midiHost are accessible externally (not static) for serial_midi.cpp
EZ_USB_MIDI_HOST<MidiHostSettingsDefault> myMidiHost;
EZ_USB_MIDI_HOST<MidiHostSettingsDefault>& midiHost = myMidiHost; // This reference should be fine


static bool core0_booting = true;
static bool core1_booting = true;

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

// Function pointers for USB Host MIDI callbacks (remain the same)
NoteOffFunctionPtr onNoteOff = usbh_onNoteOffHandle;
NoteOnFunctionPtr onNoteOn = usbh_onNoteOnHandle;
PolyphonicAftertouchFunctionPtr onPolyphonicAftertouch = usbh_onPolyphonicAftertouchHandle;
ControlChangeFunctionPtr onControlChange = usbh_onControlChangeHandle;
ProgramChangeFunctionPtr onProgramChange = usbh_onProgramChangeHandle;
AftertouchFunctionPtr onAftertouch = usbh_onAftertouchHandle;
PitchBendFunctionPtr onPitchBend = usbh_onPitchBendHandle;
SysExFunctionPtr onSysEx = usbh_onSysExHandle;
MidiClockFunctionPtr onMidiClock = usbh_onMidiClockHandle;
MidiStartFunctionPtr onMidiStart = usbh_onMidiStartHandle;
MidiContinueFunctionPtr onMidiContinue = usbh_onMidiContinueHandle;
MidiStopFunctionPtr onMidiStop = usbh_onMidiStopHandle;

// Setup function for core 0
void setup() {
  // Configure LED pins
  pinMode(LED_IN_PIN, OUTPUT);
  pinMode(LED_OUT_PIN, OUTPUT);
  digitalWrite(LED_IN_PIN, LOW);
  digitalWrite(LED_OUT_PIN, LOW);
  initLEDs();

  // Configure Serial1 pins - MOVED TO setupSerialMidi()
  // Serial1.setRX(serialRxPin);
  // Serial1.setTX(serialTxPin);

  // Initialize USB MIDI device
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
  // SERIAL_M.begin(MIDI_CHANNEL_OMNI);
  // SERIAL_M.setHandleNoteOn(serial_onNoteOn);
  // ... (remove all SERIAL_M.setHandle... calls)

  // Initialize debug serial
  Serial.begin(115200);
  while (!Serial) {
    delay(100);   // wait for native usb
  }

  // wait until device mounted
  while(!TinyUSBDevice.mounted()) delay(1);

  Serial.println("RP2040 USB MIDI Router - Main Sketch");
  Serial.println("Forwarding MIDI between USB Host and USB Device");

  // Initialize MIDI filters
  setupMidiFilters();
  
  // Initialize EEPROM and load filter settings
  setupEEPROM();
  
  // Call the setup function for the Serial MIDI module
  setupSerialMidi();

  core0_booting = false;
  while(core1_booting);


  USB_D.turnThruOff();
}

// Main loop for core 0
void loop() {
  // Process USB Host MIDI
  myMidiHost.readAll();
  myMidiHost.writeFlushAll();

  // Process USB Device MIDI
  USB_D.read();

  // Process Serial MIDI
  loopSerialMidi(); 
  
  // Process serial commands for EEPROM configuration
  processSerialCommands();

  // Handle LED indicators
  handleLEDs();
}

// Setup function for core 1 (USB Host) - Remains the same
void setup1() {
  while(!Serial);   // wait for native usb
  Serial.println("Core1 setup to run TinyUSB host with pio-usb");

  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = clock_get_hz(clk_sys);
  if (cpu_hz != 120000000UL && cpu_hz != 240000000UL) {
    delay(2000);   // wait for native usb
    Serial.printf("Error: CPU Clock = %u, PIO USB require CPU clock must be multiple of 120 Mhz\r\n", cpu_hz);
    Serial.printf("Change your CPU Clock to either 120 or 240 Mhz in Menu->CPU Speed\r\n");
    while(1) delay(1);
  }

  // Configure PIO USB
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = HOST_PIN_DP;

  #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    /* Need to swap PIOs so PIO code from CYW43 PIO SPI driver will fit */
    pio_cfg.pio_rx_num = 0;
    pio_cfg.pio_tx_num = 1;
  #endif

  USBHost.configure_pio_usb(1, &pio_cfg);

  // Initialize USB Host MIDI
  myMidiHost.begin(&USBHost, 1, onMIDIconnect, onMIDIdisconnect);
  core1_booting = false;
  while(core0_booting);
}

// Main loop for core 1 - Remains the same
void loop1() {
  USBHost.task();
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
  Serial.printf("USB Host: Note Off - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_NOTE_OFF)) {
    USB_D.sendNoteOff(note, velocity, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_NOTE_OFF)) {
    sendSerialMidiNoteOff(channel, note, velocity);
  }
  
  triggerUsbLED();
}

void usbh_onNoteOnHandle(byte channel, byte note, byte velocity) {
  Serial.printf("USB Host: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_NOTE_ON)) {
    USB_D.sendNoteOn(note, velocity, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_NOTE_ON)) {
    sendSerialMidiNoteOn(channel, note, velocity);
  }
  
  triggerUsbLED();
}

void usbh_onPolyphonicAftertouchHandle(byte channel, byte note, byte amount) {
  Serial.printf("USB Host: Poly Aftertouch - Channel: %d, Note: %d, Amount: %d\n", channel, note, amount);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_POLY_AFTERTOUCH)) {
    USB_D.sendAfterTouch(note, amount, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_POLY_AFTERTOUCH)) {
    sendSerialMidiAfterTouch(channel, note, amount);
  }
  
  triggerUsbLED();
}

void usbh_onControlChangeHandle(byte channel, byte controller, byte value) {
  Serial.printf("USB Host: CC - Channel: %d, Controller: %d, Value: %d\n", channel, controller, value);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_CONTROL_CHANGE)) {
    USB_D.sendControlChange(controller, value, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_CONTROL_CHANGE)) {
    sendSerialMidiControlChange(channel, controller, value);
  }
  
  triggerUsbLED();
}

void usbh_onProgramChangeHandle(byte channel, byte program) {
  Serial.printf("USB Host: Program Change - Channel: %d, Program: %d\n", channel, program);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_PROGRAM_CHANGE)) {
    USB_D.sendProgramChange(program, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_PROGRAM_CHANGE)) {
    sendSerialMidiProgramChange(channel, program);
  }
  
  triggerUsbLED();
}

void usbh_onAftertouchHandle(byte channel, byte value) { // Channel Aftertouch
  Serial.printf("USB Host: Channel Aftertouch - Channel: %d, Value: %d\n", channel, value);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_CHANNEL_AFTERTOUCH)) {
    USB_D.sendAfterTouch(value, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_CHANNEL_AFTERTOUCH)) {
    sendSerialMidiAfterTouchChannel(channel, value);
  }
  
  triggerUsbLED();
}

void usbh_onPitchBendHandle(byte channel, int value) {
  Serial.printf("USB Host: Pitch Bend - Channel: %d, Value: %d\n", channel, value);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_PITCH_BEND)) {
    USB_D.sendPitchBend(value, channel);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_PITCH_BEND)) {
    sendSerialMidiPitchBend(channel, value);
  }
  
  triggerUsbLED();
}

void usbh_onSysExHandle(byte * array, unsigned size) {
  Serial.printf("USB Host: SysEx - Size: %d\n", size);
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_SYSEX)) {
    USB_D.sendSysEx(size, array);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_SYSEX)) {
    sendSerialMidiSysEx(size, array);
  }
  
  triggerUsbLED();
}

void usbh_onMidiClockHandle() {
  // Avoid printing every clock message
  // Serial.println("USB Host: MIDI Clock");
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_REALTIME)) {
    USB_D.sendRealTime(midi::Clock);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Clock);
  }
  
  triggerUsbLED();
}

void usbh_onMidiStartHandle() {
  Serial.println("USB Host: MIDI Start");
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_REALTIME)) {
    USB_D.sendRealTime(midi::Start);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Start);
  }
  
  triggerUsbLED();
}

void usbh_onMidiContinueHandle() {
  Serial.println("USB Host: MIDI Continue");
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_REALTIME)) {
    USB_D.sendRealTime(midi::Continue);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Continue);
  }
  
  triggerUsbLED();
}

void usbh_onMidiStopHandle() {
  Serial.println("USB Host: MIDI Stop");
  
  // Forward to USB Device MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_DEVICE, (MidiMsgType)MIDI_MSG_REALTIME)) {
    USB_D.sendRealTime(midi::Stop);
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Stop);
  }
  
  triggerUsbLED();
}

// --- USB Device MIDI message handlers ---
// These forward messages to USB Host MIDI and Serial MIDI
// MODIFIED to use sendSerialMidi... functions

void usbd_onNoteOn(byte channel, byte note, byte velocity) {
  Serial.printf("USB Device: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_NOTE_ON)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendNoteOn(note, velocity, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_NOTE_ON)) {
    sendSerialMidiNoteOn(channel, note, velocity);
  }
  
  triggerUsbLED();
}

void usbd_onNoteOff(byte channel, byte note, byte velocity) {
  Serial.printf("USB Device: Note Off - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_NOTE_OFF)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendNoteOff(note, velocity, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_NOTE_OFF)) {
    sendSerialMidiNoteOff(channel, note, velocity);
  }
  
  triggerUsbLED();
}

void usbd_onControlChange(byte channel, byte controller, byte value) {
  Serial.printf("USB Device: CC - Channel: %d, Controller: %d, Value: %d\n", channel, controller, value);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_CONTROL_CHANGE)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendControlChange(controller, value, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_CONTROL_CHANGE)) {
    sendSerialMidiControlChange(channel, controller, value);
  }
  
  triggerUsbLED();
}

void usbd_onProgramChange(byte channel, byte program) {
  Serial.printf("USB Device: Program Change - Channel: %d, Program: %d\n", channel, program);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_PROGRAM_CHANGE)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendProgramChange(program, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_PROGRAM_CHANGE)) {
    sendSerialMidiProgramChange(channel, program);
  }
  
  triggerUsbLED();
}

void usbd_onAftertouch(byte channel, byte pressure) { // Channel Aftertouch
  Serial.printf("USB Device: Channel Aftertouch - Channel: %d, Pressure: %d\n", channel, pressure);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_CHANNEL_AFTERTOUCH)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendAfterTouch(pressure, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_CHANNEL_AFTERTOUCH)) {
    sendSerialMidiAfterTouchChannel(channel, pressure);
  }
  
  triggerUsbLED();
}

void usbd_onPitchBend(byte channel, int bend) {
  Serial.printf("USB Device: Pitch Bend - Channel: %d, Bend: %d\n", channel, bend);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_PITCH_BEND)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendPitchBend(bend, channel);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_PITCH_BEND)) {
    sendSerialMidiPitchBend(channel, bend);
  }
  
  triggerUsbLED();
}

void usbd_onSysEx(byte * array, unsigned size) {
  Serial.printf("USB Device: SysEx - Size: %d\n", size);
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_SYSEX)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendSysEx(size, array);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_SYSEX)) {
    sendSerialMidiSysEx(size, array);
  }
  
  triggerUsbLED();
}

// USB Device MIDI real-time message handlers
void usbd_onClock() {
  // Avoid printing every clock message
  // Serial.println("USB Device: MIDI Clock");
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_REALTIME)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendRealTime(midi::Clock);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Clock);
  }
  
  triggerUsbLED();
}

void usbd_onStart() {
  Serial.println("USB Device: MIDI Start");
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_REALTIME)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendRealTime(midi::Start);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Start);
  }
  
  triggerUsbLED();
}

void usbd_onContinue() {
  Serial.println("USB Device: MIDI Continue");
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_REALTIME)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendRealTime(midi::Continue);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Continue);
  }
  
  triggerUsbLED();
}

void usbd_onStop() {
  Serial.println("USB Device: MIDI Stop");
  
  // Forward to USB Host MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_USB_HOST, (MidiMsgType)MIDI_MSG_REALTIME)) {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
      intf->sendRealTime(midi::Stop);
    }
  }
  
  // Forward to Serial MIDI if not filtered
  if (!isMidiFiltered((MidiInterfaceType)MIDI_INTERFACE_SERIAL, (MidiMsgType)MIDI_MSG_REALTIME)) {
    sendSerialMidiRealTime(midi::Stop);
  }
  
  triggerUsbLED();
}

// Serial MIDI message handlers - REMOVED (defined in serial_midi.cpp)
// void serial_onNoteOn(...) { ... }
// ... (remove all serial_on... definitions)
