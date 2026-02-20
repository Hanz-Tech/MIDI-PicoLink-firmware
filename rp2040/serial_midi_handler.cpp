#include "serial_midi_handler.h"
#include <HardwareSerial.h> // Needed for HardwareSerial object (Serial1)
#include "usb_host_wrapper.h" // For midi_dev_addr and USB host functions
#include "led_utils.h" // For triggerSerialLED()
#include "midi_instances.h"
#include "midi_filters.h" // Include the MIDI filters
#include "midi_router.h"
#include "serial_utils.h" // Include the dual printing utilities
#include "pin_config.h"

// --- MIDI Instances ---
// Create Serial MIDI instance using Serial1
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SERIAL_M);

// --- External References ---
// These objects are defined in the main sketch or other included files
// and are needed here to forward MIDI messages *from* Serial MIDI *to* USB Host and Device.
extern volatile uint8_t midi_dev_addr; // Set by USB Host connection callback
extern volatile bool midi_host_mounted; // Set by USB Host connection callback
// USB_D is now created by the macro in midi_instances.h


// --- Local Handler Forward Declarations ---
// These handlers are called by the MIDI library when messages arrive *on* Serial MIDI
void serial_onNoteOn(byte channel, byte note, byte velocity);
void serial_onNoteOff(byte channel, byte note, byte velocity);
void serial_onControlChange(byte channel, byte controller, byte value);
void serial_onProgramChange(byte channel, byte program);
void serial_onChannelAftertouch(byte channel, byte pressure); // Channel Aftertouch
// Note: The MIDI library doesn't directly support Polyphonic Aftertouch *input* handling
// via a dedicated setHandle function like it does for output.
// If you need to handle incoming Poly AT on Serial, you might need custom parsing
// or check if the library version you use has added it.
// We'll omit a specific handler for incoming Poly AT from Serial for now.
void serial_onPitchBend(byte channel, int bend);
void serial_onSysEx(byte * array, unsigned size);
void serial_onClock();
void serial_onStart();
void serial_onContinue();
void serial_onStop();

// --- Public Functions (declared in .h) ---

void setupSerialMidi() {
    // Configure Serial1 pins
    Serial1.setRX(SERIAL_MIDI_RX_PIN);
    Serial1.setTX(SERIAL_MIDI_TX_PIN);

    // Initialize Serial MIDI
    SERIAL_M.begin(MIDI_CHANNEL_OMNI);
    SERIAL_M.setHandleNoteOn(serial_onNoteOn);
    SERIAL_M.setHandleNoteOff(serial_onNoteOff);
    SERIAL_M.setHandleControlChange(serial_onControlChange);
    SERIAL_M.setHandleProgramChange(serial_onProgramChange);
    SERIAL_M.setHandleAfterTouchChannel(serial_onChannelAftertouch); // Channel Aftertouch
    SERIAL_M.setHandlePitchBend(serial_onPitchBend);
    SERIAL_M.setHandleSystemExclusive(serial_onSysEx);
    SERIAL_M.setHandleClock(serial_onClock);
    SERIAL_M.setHandleStart(serial_onStart);
    SERIAL_M.setHandleContinue(serial_onContinue);
    SERIAL_M.setHandleStop(serial_onStop);
    SERIAL_M.turnThruOff();

    dualPrintf("Serial MIDI Module: Initialized using pins: RX=%d, TX=%d\n", SERIAL_MIDI_RX_PIN, SERIAL_MIDI_TX_PIN);
    dualPrintln("");
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

void serial_onNoteOn(byte channel, byte note, byte velocity) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_NOTE;
    msg.subType = (velocity == 0) ? 1 : 0;
    msg.channel = channel;
    msg.data1 = note;
    msg.data2 = velocity;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onNoteOff(byte channel, byte note, byte velocity) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_NOTE;
    msg.subType = 1;
    msg.channel = channel;
    msg.data1 = note;
    msg.data2 = velocity;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onControlChange(byte channel, byte controller, byte value) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_CONTROL_CHANGE;
    msg.channel = channel;
    msg.data1 = controller;
    msg.data2 = value;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onProgramChange(byte channel, byte program) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_PROGRAM_CHANGE;
    msg.channel = channel;
    msg.data1 = program;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onChannelAftertouch(byte channel, byte pressure) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_CHANNEL_AFTERTOUCH;
    msg.channel = channel;
    msg.data1 = pressure;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onPitchBend(byte channel, int bend) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_PITCH_BEND;
    msg.channel = channel;
    msg.pitchBend = bend;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onSysEx(byte * array, unsigned size) {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_SYSEX;
    msg.channel = 0;
    msg.sysexData = array;
    msg.sysexSize = size;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onClock() {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_REALTIME;
    msg.channel = 0;
    msg.rtType = midi::Clock;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}


void serial_onStart() {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_REALTIME;
    msg.channel = 0;
    msg.rtType = midi::Start;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onContinue() {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_REALTIME;
    msg.channel = 0;
    msg.rtType = midi::Continue;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}

void serial_onStop() {
    MidiMessage msg = {};
    msg.type = MIDI_MSG_REALTIME;
    msg.channel = 0;
    msg.rtType = midi::Stop;
    routeMidiMessage(MIDI_INTERFACE_SERIAL, msg);
}
