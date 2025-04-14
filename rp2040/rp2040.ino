#include <usb_midi_host.h>
#include "pio_usb_configuration.h"
#include <Arduino.h>
#include <MIDI.h>
#include "usb_host_wrapper.h"
#include "pio_usb.h"

// USB Host configuration
#define HOST_PIN_DP   15   // Pin used as D+ for host, D- = D+ + 1
#include "EZ_USB_MIDI_HOST.h"


#define LED_IN_PIN 29
#define LED_OUT_PIN 19

// Serial MIDI pins configuration - can be changed as needed
int serialRxPin = 1;  // GPIO pin for Serial1 RX
int serialTxPin = 0;  // GPIO pin for Serial1 TX

// USB MIDI device address (set by onMIDIconnect callback in usb_host_wrapper.cpp)
extern uint8_t midi_dev_addr;

// USB Host object
Adafruit_USBH_Host USBHost;

// USB Device MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create USB Device MIDI instance
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USB_D);

// Create Serial MIDI instance using Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SERIAL_M);

// USB Host MIDI
USING_NAMESPACE_MIDI
USING_NAMESPACE_EZ_USB_MIDI_HOST

EZ_USB_MIDI_HOST<MidiHostSettingsDefault> myMidiHost;
EZ_USB_MIDI_HOST<MidiHostSettingsDefault>& midiHost = myMidiHost;

static bool core0_booting = true;
static bool core1_booting = true;

// Forward declarations for MIDI message handlers
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

// USB Device MIDI message handlers
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

// Serial MIDI message handlers
void serial_onNoteOn(byte channel, byte note, byte velocity);
void serial_onNoteOff(byte channel, byte note, byte velocity);
void serial_onControlChange(byte channel, byte controller, byte value);
void serial_onProgramChange(byte channel, byte program);
void serial_onAftertouch(byte channel, byte pressure);
void serial_onPitchBend(byte channel, int bend);
void serial_onSysEx(byte * array, unsigned size);
void serial_onClock();
void serial_onStart();
void serial_onContinue();
void serial_onStop();

// Function pointers for USB Host MIDI callbacks
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
  
  // Configure Serial1 pins
  Serial1.setRX(serialRxPin);
  Serial1.setTX(serialTxPin);
  
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
  
  // Initialize Serial MIDI
  SERIAL_M.begin(MIDI_CHANNEL_OMNI);
  SERIAL_M.setHandleNoteOn(serial_onNoteOn);
  SERIAL_M.setHandleNoteOff(serial_onNoteOff);
  SERIAL_M.setHandleControlChange(serial_onControlChange);
  SERIAL_M.setHandleProgramChange(serial_onProgramChange);
  SERIAL_M.setHandleAfterTouchChannel(serial_onAftertouch);
  SERIAL_M.setHandlePitchBend(serial_onPitchBend);
  SERIAL_M.setHandleSystemExclusive(serial_onSysEx);
  SERIAL_M.setHandleClock(serial_onClock);
  SERIAL_M.setHandleStart(serial_onStart);
  SERIAL_M.setHandleContinue(serial_onContinue);
  SERIAL_M.setHandleStop(serial_onStop);
  
  // Initialize debug serial
  Serial.begin(115200);
  while (!Serial) {
    delay(100);   // wait for native usb
  }

  // wait until device mounted
  while(!TinyUSBDevice.mounted()) delay(1);

  Serial.println("RP2040 USB MIDI Router");
  Serial.println("Forwarding MIDI between USB Host, USB Device, and Serial MIDI");
  Serial.printf("Serial MIDI using pins: RX=%d, TX=%d\n", serialRxPin, serialTxPin);
  
  core0_booting = false;
  while(core1_booting);
}

// Main loop for core 0
void loop() {
  // Process USB Host MIDI
  myMidiHost.readAll();
  myMidiHost.writeFlushAll();
  
  // Process USB Device MIDI
  USB_D.read();
  
  // Process Serial MIDI
  SERIAL_M.read();
  
  // Handle LED indicators
  handleLEDs();
}

// Setup function for core 1 (USB Host)
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

// Main loop for core 1
void loop1() {
  USBHost.task();
}

// Shared variables for LED state
static uint32_t inLedStartMs = 0;
static bool inLedActive = false;
static uint32_t outLedStartMs = 0;
static bool outLedActive = false;

// LED handling functions for MIDI activity
static void handleLEDs() {
  // Handle LED_IN_PIN (for incoming serial MIDI)
  // If LED is active and 50ms has passed, turn it off
  if (inLedActive && (millis() - inLedStartMs >= 50)) {
    digitalWrite(LED_IN_PIN, LOW);
    inLedActive = false;
  }
  
  // Handle LED_OUT_PIN (for outgoing serial MIDI)
  // If LED is active and 50ms has passed, turn it off
  if (outLedActive && (millis() - outLedStartMs >= 50)) {
    digitalWrite(LED_OUT_PIN, LOW);
    outLedActive = false;
  }
}

// Function to trigger the input LED for 50ms
static void triggerInLED() {
  // Turn on LED and record the time
  digitalWrite(LED_IN_PIN, HIGH);
  inLedStartMs = millis();
  inLedActive = true;
}

// Function to trigger the output LED for 50ms
static void triggerOutLED() {
  // Turn on LED and record the time
  digitalWrite(LED_OUT_PIN, HIGH);
  outLedStartMs = millis();
  outLedActive = true;
}

// USB Host MIDI message handlers
// These forward messages to USB Device MIDI and Serial MIDI

void usbh_onNoteOffHandle(byte channel, byte note, byte velocity) {
  Serial.printf("USB Host: Note Off - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Device MIDI
  USB_D.sendNoteOff(note, velocity, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendNoteOff(note, velocity, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onNoteOnHandle(byte channel, byte note, byte velocity) {
  Serial.printf("USB Host: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Device MIDI
  USB_D.sendNoteOn(note, velocity, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendNoteOn(note, velocity, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onPolyphonicAftertouchHandle(byte channel, byte note, byte amount) {
  Serial.printf("USB Host: Poly Aftertouch - Channel: %d, Note: %d, Amount: %d\n", channel, note, amount);
  
  // Forward to USB Device MIDI
  USB_D.sendAfterTouch(note, amount, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendAfterTouch(note, amount, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onControlChangeHandle(byte channel, byte controller, byte value) {
  Serial.printf("USB Host: CC - Channel: %d, Controller: %d, Value: %d\n", channel, controller, value);
  
  // Forward to USB Device MIDI
  USB_D.sendControlChange(controller, value, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendControlChange(controller, value, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onProgramChangeHandle(byte channel, byte program) {
  Serial.printf("USB Host: Program Change - Channel: %d, Program: %d\n", channel, program);
  
  // Forward to USB Device MIDI
  USB_D.sendProgramChange(program, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendProgramChange(program, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onAftertouchHandle(byte channel, byte value) {
  Serial.printf("USB Host: Channel Aftertouch - Channel: %d, Value: %d\n", channel, value);
  
  // Forward to USB Device MIDI
  USB_D.sendAfterTouch(value, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendAfterTouch(value, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onPitchBendHandle(byte channel, int value) {
  Serial.printf("USB Host: Pitch Bend - Channel: %d, Value: %d\n", channel, value);
  
  // Forward to USB Device MIDI
  USB_D.sendPitchBend(value, channel);
  
  // Forward to Serial MIDI
  SERIAL_M.sendPitchBend(value, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onSysExHandle(byte * array, unsigned size) {
  Serial.printf("USB Host: SysEx - Size: %d\n", size);
  
  // Forward to USB Device MIDI
  USB_D.sendSysEx(size, array);
  
  // Forward to Serial MIDI
  SERIAL_M.sendSysEx(size, array);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onMidiClockHandle() {
  Serial.println("USB Host: MIDI Clock");
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Clock);
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Clock);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onMidiStartHandle() {
  Serial.println("USB Host: MIDI Start");
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Start);
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Start);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onMidiContinueHandle() {
  Serial.println("USB Host: MIDI Continue");
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Continue);
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Continue);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbh_onMidiStopHandle() {
  Serial.println("USB Host: MIDI Stop");
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Stop);
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Stop);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

// USB Device MIDI message handlers
// These forward messages to USB Host MIDI and Serial MIDI

void usbd_onNoteOn(byte channel, byte note, byte velocity) {
  Serial.printf("USB Device: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendNoteOn(note, velocity, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendNoteOn(note, velocity, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onNoteOff(byte channel, byte note, byte velocity) {
  Serial.printf("USB Device: Note Off - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendNoteOff(note, velocity, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendNoteOff(note, velocity, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onControlChange(byte channel, byte controller, byte value) {
  Serial.printf("USB Device: CC - Channel: %d, Controller: %d, Value: %d\n", channel, controller, value);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendControlChange(controller, value, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendControlChange(controller, value, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onProgramChange(byte channel, byte program) {
  Serial.printf("USB Device: Program Change - Channel: %d, Program: %d\n", channel, program);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendProgramChange(program, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendProgramChange(program, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onAftertouch(byte channel, byte pressure) {
  Serial.printf("USB Device: Channel Aftertouch - Channel: %d, Pressure: %d\n", channel, pressure);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendAfterTouch(pressure, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendAfterTouch(pressure, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onPitchBend(byte channel, int bend) {
  Serial.printf("USB Device: Pitch Bend - Channel: %d, Bend: %d\n", channel, bend);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendPitchBend(bend, channel);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendPitchBend(bend, channel);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onSysEx(byte * array, unsigned size) {
  Serial.printf("USB Device: SysEx - Size: %d\n", size);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendSysEx(size, array);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendSysEx(size, array);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

// USB Device MIDI real-time message handlers
// These forward messages to USB Host MIDI and Serial MIDI

void usbd_onClock() {
  Serial.println("USB Device: MIDI Clock");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Clock);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Clock);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onStart() {
  Serial.println("USB Device: MIDI Start");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Start);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Start);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onContinue() {
  Serial.println("USB Device: MIDI Continue");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Continue);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Continue);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

void usbd_onStop() {
  Serial.println("USB Device: MIDI Stop");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Stop);
  }
  
  // Forward to Serial MIDI
  SERIAL_M.sendRealTime(midi::Stop);
  triggerOutLED(); // Trigger output LED for outgoing serial MIDI activity
}

// Serial MIDI message handlers
// These forward messages to USB Host MIDI and USB Device MIDI

void serial_onNoteOn(byte channel, byte note, byte velocity) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: Note On - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendNoteOn(note, velocity, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendNoteOn(note, velocity, channel);
}

void serial_onNoteOff(byte channel, byte note, byte velocity) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: Note Off - Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendNoteOff(note, velocity, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendNoteOff(note, velocity, channel);
}

void serial_onControlChange(byte channel, byte controller, byte value) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: CC - Channel: %d, Controller: %d, Value: %d\n", channel, controller, value);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendControlChange(controller, value, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendControlChange(controller, value, channel);
}

void serial_onProgramChange(byte channel, byte program) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: Program Change - Channel: %d, Program: %d\n", channel, program);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendProgramChange(program, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendProgramChange(program, channel);
}

void serial_onAftertouch(byte channel, byte pressure) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: Channel Aftertouch - Channel: %d, Pressure: %d\n", channel, pressure);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendAfterTouch(pressure, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendAfterTouch(pressure, channel);
}

void serial_onPitchBend(byte channel, int bend) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: Pitch Bend - Channel: %d, Bend: %d\n", channel, bend);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendPitchBend(bend, channel);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendPitchBend(bend, channel);
}

void serial_onSysEx(byte * array, unsigned size) {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.printf("Serial MIDI: SysEx - Size: %d\n", size);
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendSysEx(size, array);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendSysEx(size, array);
}

void serial_onClock() {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.println("Serial MIDI: Clock");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Clock);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Clock);
}

void serial_onStart() {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.println("Serial MIDI: Start");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Start);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Start);
}

void serial_onContinue() {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.println("Serial MIDI: Continue");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Continue);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Continue);
}

void serial_onStop() {
  // Trigger input LED for incoming serial MIDI activity
  triggerInLED();
  
  Serial.println("Serial MIDI: Stop");
  
  // Forward to USB Host MIDI
  auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
  if (intf != nullptr) {
    intf->sendRealTime(midi::Stop);
  }
  
  // Forward to USB Device MIDI
  USB_D.sendRealTime(midi::Stop);
}
