#include "usb_host_midi_handlers.h"
#include "midi_router.h"
#include <MIDI.h>

USING_NAMESPACE_MIDI

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

void usbh_onSysExHandle(byte *array, unsigned size) {
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

void onNoteOff(Channel channel, byte note, byte velocity) {
  usbh_onNoteOffHandle(channel, note, velocity);
}

void onNoteOn(Channel channel, byte note, byte velocity) {
  usbh_onNoteOnHandle(channel, note, velocity);
}

void onPolyphonicAftertouch(Channel channel, byte note, byte pressure) {
  usbh_onPolyphonicAftertouchHandle(channel, note, pressure);
}

void onControlChange(Channel channel, byte control, byte value) {
  usbh_onControlChangeHandle(channel, control, value);
}

void onProgramChange(Channel channel, byte program) {
  usbh_onProgramChangeHandle(channel, program);
}

void onAftertouch(Channel channel, byte pressure) {
  usbh_onAftertouchHandle(channel, pressure);
}

void onPitchBend(Channel channel, int bend) {
  usbh_onPitchBendHandle(channel, bend);
}

void onSysEx(byte *array, unsigned size) {
  usbh_onSysExHandle(array, size);
}

void onMidiClock() {
  usbh_onMidiClockHandle();
}

void onMidiStart() {
  usbh_onMidiStartHandle();
}

void onMidiContinue() {
  usbh_onMidiContinueHandle();
}

void onMidiStop() {
  usbh_onMidiStopHandle();
}

void setupUsbHostHandlers() {
  // Placeholder for future USB Host handler initialization.
}
