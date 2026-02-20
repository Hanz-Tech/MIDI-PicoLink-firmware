#ifndef USB_HOST_MIDI_HANDLERS_H
#define USB_HOST_MIDI_HANDLERS_H

#include <Arduino.h>
#include <MIDI.h>

// USB Host MIDI handler functions (called by processMidiPacket in usb_host_wrapper.cpp)
void usbh_onNoteOn(byte channel, byte note, byte velocity);
void usbh_onNoteOff(byte channel, byte note, byte velocity);
void usbh_onPolyAftertouch(byte channel, byte note, byte amount);
void usbh_onControlChange(byte channel, byte controller, byte value);
void usbh_onProgramChange(byte channel, byte program);
void usbh_onChannelAftertouch(byte channel, byte value);
void usbh_onPitchBend(byte channel, int value);
void usbh_onSysEx(byte *array, unsigned size);
void usbh_onMidiClock();
void usbh_onMidiStart();
void usbh_onMidiContinue();
void usbh_onMidiStop();

// C-style wrappers (needed by usb_host_wrapper.cpp extern declarations)
// These are defined with MIDI library Channel type for TinyUSB compatibility
void onNoteOff(midi::Channel channel, byte note, byte velocity);
void onNoteOn(midi::Channel channel, byte note, byte velocity);
void onPolyphonicAftertouch(midi::Channel channel, byte note, byte pressure);
void onControlChange(midi::Channel channel, byte control, byte value);
void onProgramChange(midi::Channel channel, byte program);
void onAftertouch(midi::Channel channel, byte pressure);
void onPitchBend(midi::Channel channel, int bend);
void onSysEx(byte *array, unsigned size);
void onMidiClock();
void onMidiStart();
void onMidiContinue();
void onMidiStop();

void setupUsbHostHandlers();

#endif // USB_HOST_MIDI_HANDLERS_H
