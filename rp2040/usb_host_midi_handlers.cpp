#include "usb_host_midi_handlers.h"
#include "midi_router.h"
#include <MIDI.h>

USING_NAMESPACE_MIDI

void usbh_onNoteOff(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 1;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onNoteOn(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 0;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onPolyAftertouch(byte channel, byte note, byte amount) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_POLY_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = amount;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onControlChange(byte channel, byte controller, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CONTROL_CHANGE;
  msg.channel = channel;
  msg.data1 = controller;
  msg.data2 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onProgramChange(byte channel, byte program) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PROGRAM_CHANGE;
  msg.channel = channel;
  msg.data1 = program;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onChannelAftertouch(byte channel, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CHANNEL_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onPitchBend(byte channel, int value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PITCH_BEND;
  msg.channel = channel;
  msg.pitchBend = value;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onSysEx(byte *array, unsigned size) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_SYSEX;
  msg.channel = 0;
  msg.sysexData = array;
  msg.sysexSize = size;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiClock() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Clock;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiStart() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Start;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiContinue() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Continue;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void usbh_onMidiStop() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Stop;
  routeMidiMessage(MIDI_INTERFACE_USB_HOST, msg);
}

void onNoteOff(Channel channel, byte note, byte velocity) {
  usbh_onNoteOff(channel, note, velocity);
}

void onNoteOn(Channel channel, byte note, byte velocity) {
  usbh_onNoteOn(channel, note, velocity);
}

void onPolyphonicAftertouch(Channel channel, byte note, byte pressure) {
  usbh_onPolyAftertouch(channel, note, pressure);
}

void onControlChange(Channel channel, byte control, byte value) {
  usbh_onControlChange(channel, control, value);
}

void onProgramChange(Channel channel, byte program) {
  usbh_onProgramChange(channel, program);
}

void onAftertouch(Channel channel, byte pressure) {
  usbh_onChannelAftertouch(channel, pressure);
}

void onPitchBend(Channel channel, int bend) {
  usbh_onPitchBend(channel, bend);
}

void onSysEx(byte *array, unsigned size) {
  usbh_onSysEx(array, size);
}

void onMidiClock() {
  usbh_onMidiClock();
}

void onMidiStart() {
  usbh_onMidiStart();
}

void onMidiContinue() {
  usbh_onMidiContinue();
}

void onMidiStop() {
  usbh_onMidiStop();
}

void setupUsbHostHandlers() {
  // Placeholder for future USB Host handler initialization.
}
