#ifndef USB_HOST_MIDI_HANDLERS_H
#define USB_HOST_MIDI_HANDLERS_H

#include <Arduino.h>
#include <MIDI.h>

// USB Host MIDI handler functions (called by processMidiPacket in usb_host_wrapper.cpp)
void usbh_onNoteOnHandle(byte channel, byte note, byte velocity);
void usbh_onNoteOffHandle(byte channel, byte note, byte velocity);
void usbh_onPolyphonicAftertouchHandle(byte channel, byte note, byte amount);
void usbh_onControlChangeHandle(byte channel, byte controller, byte value);
void usbh_onProgramChangeHandle(byte channel, byte program);
void usbh_onAftertouchHandle(byte channel, byte value);
void usbh_onPitchBendHandle(byte channel, int value);
void usbh_onSysExHandle(byte *array, unsigned size);
void usbh_onMidiClockHandle();
void usbh_onMidiStartHandle();
void usbh_onMidiContinueHandle();
void usbh_onMidiStopHandle();

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
