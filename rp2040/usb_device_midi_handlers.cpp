#include "usb_device_midi_handlers.h"
#include "midi_router.h"
#include "midi_instances.h"

static void usbd_onNoteOn(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 0;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onNoteOff(byte channel, byte note, byte velocity) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_NOTE;
  msg.subType = 1;
  msg.channel = channel;
  msg.data1 = note;
  msg.data2 = velocity;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onControlChange(byte channel, byte controller, byte value) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CONTROL_CHANGE;
  msg.channel = channel;
  msg.data1 = controller;
  msg.data2 = value;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onProgramChange(byte channel, byte program) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PROGRAM_CHANGE;
  msg.channel = channel;
  msg.data1 = program;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onAftertouch(byte channel, byte pressure) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_CHANNEL_AFTERTOUCH;
  msg.channel = channel;
  msg.data1 = pressure;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onPitchBend(byte channel, int bend) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_PITCH_BEND;
  msg.channel = channel;
  msg.pitchBend = bend;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onSysEx(byte *array, unsigned size) {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_SYSEX;
  msg.channel = 0;
  msg.sysexData = array;
  msg.sysexSize = size;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onClock() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Clock;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onStart() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Start;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onContinue() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Continue;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

static void usbd_onStop() {
  MidiMessage msg = {};
  msg.type = MIDI_MSG_REALTIME;
  msg.channel = 0;
  msg.rtType = midi::Stop;
  routeMidiMessage(MIDI_INTERFACE_USB_DEVICE, msg);
}

void setupUsbDeviceHandlers() {
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
}
