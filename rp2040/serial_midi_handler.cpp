#include "serial_midi_handler.h"
#include <HardwareSerial.h> // Needed for HardwareSerial object (Serial1)
#include "usb_host_wrapper.h" // For midi_dev_addr and midiHost definition/extern
#include "EZ_USB_MIDI_HOST.h"   // For midiHost type and methods
#include "led_utils.h" // For triggerSerialLED()
#include "midi_instances.h"

// --- Configuration ---
int serialRxPin = 1;  // GPIO pin for Serial1 RX
int serialTxPin = 0;  // GPIO pin for Serial1 TX

// --- MIDI Instances ---
// Create Serial MIDI instance using Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SERIAL_M);

// --- External References ---
// These objects are defined in the main sketch or other included files
// and are needed here to forward MIDI messages *from* Serial MIDI *to* USB Host and Device.
extern EZ_USB_MIDI_HOST<MidiHostSettingsDefault>& midiHost;
extern uint8_t midi_dev_addr; // Set by USB Host connection callback
// USB_D is now created by the macro in midi_instances.h


// --- Local Handler Forward Declarations ---
// These handlers are called by the MIDI library when messages arrive *on* Serial MIDI
void localSerialOnNoteOn(byte channel, byte note, byte velocity);
void localSerialOnNoteOff(byte channel, byte note, byte velocity);
void localSerialOnControlChange(byte channel, byte controller, byte value);
void localSerialOnProgramChange(byte channel, byte program);
void localSerialOnAftertouch(byte channel, byte pressure); // Channel Aftertouch
// Note: The MIDI library doesn't directly support Polyphonic Aftertouch *input* handling
// via a dedicated setHandle function like it does for output.
// If you need to handle incoming Poly AT on Serial, you might need custom parsing
// or check if the library version you use has added it.
// We'll omit a specific handler for incoming Poly AT from Serial for now.
void localSerialOnPitchBend(byte channel, int bend);
void localSerialOnSysEx(byte * array, unsigned size);
void localSerialOnClock();
void localSerialOnStart();
void localSerialOnContinue();
void localSerialOnStop();

// --- Public Functions (declared in .h) ---

void setupSerialMidi() {
    // Configure Serial1 pins
    Serial1.setRX(serialRxPin);
    Serial1.setTX(serialTxPin);

    // Initialize Serial MIDI
    SERIAL_M.begin(MIDI_CHANNEL_OMNI);
    SERIAL_M.setHandleNoteOn(localSerialOnNoteOn);
    SERIAL_M.setHandleNoteOff(localSerialOnNoteOff);
    SERIAL_M.setHandleControlChange(localSerialOnControlChange);
    SERIAL_M.setHandleProgramChange(localSerialOnProgramChange);
    SERIAL_M.setHandleAfterTouchChannel(localSerialOnAftertouch); // Channel Aftertouch
    SERIAL_M.setHandlePitchBend(localSerialOnPitchBend);
    SERIAL_M.setHandleSystemExclusive(localSerialOnSysEx);
    SERIAL_M.setHandleClock(localSerialOnClock);
    SERIAL_M.setHandleStart(localSerialOnStart);
    SERIAL_M.setHandleContinue(localSerialOnContinue);
    SERIAL_M.setHandleStop(localSerialOnStop);

    Serial.printf("Serial MIDI Module: Initialized using pins: RX=%d, TX=%d\n", serialRxPin, serialTxPin);
}

void loopSerialMidi() {
    // Process incoming Serial MIDI messages
    SERIAL_M.read();
}

// Implement functions to *send* messages *to* the Serial MIDI port
void sendSerialMidiNoteOn(byte channel, byte note, byte velocity) {
    SERIAL_M.sendNoteOn(note, velocity, channel);
}

void sendSerialMidiNoteOff(byte channel, byte note, byte velocity) {
    SERIAL_M.sendNoteOff(note, velocity, channel);
}

void sendSerialMidiAfterTouch(byte channel, byte note, byte amount) { // Polyphonic AT
    SERIAL_M.sendAfterTouch(note, amount, channel);
}

void sendSerialMidiControlChange(byte channel, byte controller, byte value) {
    SERIAL_M.sendControlChange(controller, value, channel);
}

void sendSerialMidiProgramChange(byte channel, byte program) {
    SERIAL_M.sendProgramChange(program, channel);
}

void sendSerialMidiAfterTouchChannel(byte channel, byte pressure) { // Channel AT
    SERIAL_M.sendAfterTouch(pressure, channel);
}

void sendSerialMidiPitchBend(byte channel, int bend) {
    SERIAL_M.sendPitchBend(bend, channel);
}

void sendSerialMidiSysEx(unsigned size, const byte *array) {
    SERIAL_M.sendSysEx(size, array);
}

void sendSerialMidiRealTime(midi::MidiType type) {
    SERIAL_M.sendRealTime(type);
}


// --- Local Handler Implementations ---
// These handle messages *received from* Serial MIDI and forward them

void localSerialOnNoteOn(byte channel, byte note, byte velocity) {
    triggerSerialLED(); // Trigger LED for serial MIDI activity
    Serial.printf("Serial MIDI In: Note On - Ch: %d, Note: %d, Vel: %d\n", channel, note, velocity);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendNoteOn(note, velocity, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendNoteOn(note, velocity, channel);
}

void localSerialOnNoteOff(byte channel, byte note, byte velocity) {
    triggerSerialLED();
    Serial.printf("Serial MIDI In: Note Off - Ch: %d, Note: %d, Vel: %d\n", channel, note, velocity);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendNoteOff(note, velocity, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendNoteOff(note, velocity, channel);
}

void localSerialOnControlChange(byte channel, byte controller, byte value) {
    triggerSerialLED();
    Serial.printf("Serial MIDI In: CC - Ch: %d, CC: %d, Val: %d\n", channel, controller, value);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendControlChange(controller, value, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendControlChange(controller, value, channel);
}

void localSerialOnProgramChange(byte channel, byte program) {
    triggerSerialLED();
    Serial.printf("Serial MIDI In: Prgm Chg - Ch: %d, Pgm: %d\n", channel, program);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendProgramChange(program, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendProgramChange(program, channel);
}

void localSerialOnAftertouch(byte channel, byte pressure) { // Channel Aftertouch
    triggerSerialLED();
    Serial.printf("Serial MIDI In: Ch Aftertouch - Ch: %d, Pressure: %d\n", channel, pressure);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        // Note: EZ Host library uses sendAfterTouch for Channel AT when only pressure & channel are given
        intf->sendAfterTouch(pressure, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendAfterTouch(pressure, channel);
}

void localSerialOnPitchBend(byte channel, int bend) {
    triggerSerialLED();
    Serial.printf("Serial MIDI In: Pitch Bend - Ch: %d, Bend: %d\n", channel, bend);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendPitchBend(bend, channel);
    }

    // Forward to USB Device MIDI
    USB_D.sendPitchBend(bend, channel);
}

void localSerialOnSysEx(byte * array, unsigned size) {
    triggerSerialLED();
    Serial.printf("Serial MIDI In: SysEx - Size: %d\n", size);

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendSysEx(size, array);
    }

    // Forward to USB Device MIDI
    USB_D.sendSysEx(size, array);
}

void localSerialOnClock() {
    triggerSerialLED();
    // Avoid printing every clock message to prevent flooding Serial monitor
    // Serial.println("Serial MIDI In: Clock");

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendRealTime(midi::Clock);
    }

    // Forward to USB Device MIDI
    USB_D.sendRealTime(midi::Clock);
}

void localSerialOnStart() {
    triggerSerialLED();
    Serial.println("Serial MIDI In: Start");

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendRealTime(midi::Start);
    }

    // Forward to USB Device MIDI
    USB_D.sendRealTime(midi::Start);
}

void localSerialOnContinue() {
    triggerSerialLED();
    Serial.println("Serial MIDI In: Continue");

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendRealTime(midi::Continue);
    }

    // Forward to USB Device MIDI
    USB_D.sendRealTime(midi::Continue);
}

void localSerialOnStop() {
    triggerSerialLED();
    Serial.println("Serial MIDI In: Stop");

    // Forward to USB Host MIDI
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf != nullptr) {
        intf->sendRealTime(midi::Stop);
    }

    // Forward to USB Device MIDI
    USB_D.sendRealTime(midi::Stop);
}