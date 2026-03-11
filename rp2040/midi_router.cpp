#include "midi_router.h"
#include "midi_filters.h"
#include "usb_host_wrapper.h"
#include "serial_midi_handler.h"
#include "midi_instances.h"
#include "led_utils.h"
#include "serial_utils.h"

extern volatile bool isConnectedToComputer;

static uint8_t realTimeByteFromType(midi::MidiType type) {
    switch (type) {
        case midi::Clock:
            return 0xF8;
        case midi::Start:
            return 0xFA;
        case midi::Continue:
            return 0xFB;
        case midi::Stop:
            return 0xFC;
        default:
            return 0xF8;
    }
}

static void forwardToInterface(MidiInterfaceType dest, const MidiMessage &msg) {
    switch (dest) {
        case MIDI_INTERFACE_USB_HOST: {
            switch (msg.type) {
                case MIDI_MSG_NOTE:
                    if (msg.subType == 1) {
                        sendNoteOff(msg.channel, msg.data1, msg.data2);
                    } else {
                        sendNoteOn(msg.channel, msg.data1, msg.data2);
                    }
                    break;
                case MIDI_MSG_POLY_AFTERTOUCH:
                    sendPolyAfterTouch(msg.channel, msg.data1, msg.data2);
                    break;
                case MIDI_MSG_CONTROL_CHANGE:
                    sendControlChange(msg.channel, msg.data1, msg.data2);
                    break;
                case MIDI_MSG_PROGRAM_CHANGE:
                    sendProgramChange(msg.channel, msg.data1);
                    break;
                case MIDI_MSG_CHANNEL_AFTERTOUCH:
                    sendAfterTouch(msg.channel, msg.data1);
                    break;
                case MIDI_MSG_PITCH_BEND:
                    sendPitchBend(msg.channel, msg.pitchBend);
                    break;
                case MIDI_MSG_SYSEX:
                    sendSysEx(msg.sysexSize, msg.sysexData);
                    break;
                case MIDI_MSG_REALTIME:
                    sendRealTime(realTimeByteFromType(msg.rtType));
                    break;
                default:
                    break;
            }
            break;
        }
        case MIDI_INTERFACE_USB_DEVICE: {
            switch (msg.type) {
                case MIDI_MSG_NOTE:
                    if (msg.subType == 1) {
                        USB_D.sendNoteOff(msg.data1, msg.data2, msg.channel);
                    } else {
                        USB_D.sendNoteOn(msg.data1, msg.data2, msg.channel);
                    }
                    break;
                case MIDI_MSG_POLY_AFTERTOUCH:
                    USB_D.sendAfterTouch(msg.data1, msg.data2, msg.channel);
                    break;
                case MIDI_MSG_CONTROL_CHANGE:
                    USB_D.sendControlChange(msg.data1, msg.data2, msg.channel);
                    break;
                case MIDI_MSG_PROGRAM_CHANGE:
                    USB_D.sendProgramChange(msg.data1, msg.channel);
                    break;
                case MIDI_MSG_CHANNEL_AFTERTOUCH:
                    USB_D.sendAfterTouch(msg.data1, msg.channel);
                    break;
                case MIDI_MSG_PITCH_BEND:
                    USB_D.sendPitchBend(msg.pitchBend, msg.channel);
                    break;
                case MIDI_MSG_SYSEX:
                    USB_D.sendSysEx(msg.sysexSize, msg.sysexData);
                    break;
                case MIDI_MSG_REALTIME:
                    USB_D.sendRealTime(msg.rtType);
                    break;
                default:
                    break;
            }
            break;
        }
        case MIDI_INTERFACE_SERIAL: {
            switch (msg.type) {
                case MIDI_MSG_NOTE:
                    if (msg.subType == 1) {
                        sendSerialMidiNoteOff(msg.channel, msg.data1, msg.data2);
                    } else {
                        sendSerialMidiNoteOn(msg.channel, msg.data1, msg.data2);
                    }
                    break;
                case MIDI_MSG_POLY_AFTERTOUCH:
                    sendSerialMidiAfterTouch(msg.channel, msg.data1, msg.data2);
                    break;
                case MIDI_MSG_CONTROL_CHANGE:
                    sendSerialMidiControlChange(msg.channel, msg.data1, msg.data2);
                    break;
                case MIDI_MSG_PROGRAM_CHANGE:
                    sendSerialMidiProgramChange(msg.channel, msg.data1);
                    break;
                case MIDI_MSG_CHANNEL_AFTERTOUCH:
                    sendSerialMidiAfterTouchChannel(msg.channel, msg.data1);
                    break;
                case MIDI_MSG_PITCH_BEND:
                    sendSerialMidiPitchBend(msg.channel, msg.pitchBend);
                    break;
                case MIDI_MSG_SYSEX:
                    sendSerialMidiSysEx(msg.sysexSize, msg.sysexData);
                    break;
                case MIDI_MSG_REALTIME:
                    sendSerialMidiRealTime(msg.rtType);
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

void routeMidiMessage(MidiSource source, const MidiMessage &msg, byte destMask) {
    if (msg.type != MIDI_MSG_SYSEX && msg.type != MIDI_MSG_REALTIME) {
        if (msg.channel != 0 && !isChannelEnabled(msg.channel)) {
            return;
        }
    }

    if (source != MIDI_SOURCE_INTERNAL) {
        if (isMidiFiltered(static_cast<MidiInterfaceType>(source), msg.type)) {
            return;
        }
    }

    struct DestEntry {
        MidiInterfaceType iface;
        byte mask;
    };

    const DestEntry interfaces[] = {
        { MIDI_INTERFACE_USB_HOST, ROUTE_TO_USB_HOST },
        { MIDI_INTERFACE_USB_DEVICE, ROUTE_TO_USB_DEVICE },
        { MIDI_INTERFACE_SERIAL, ROUTE_TO_SERIAL }
    };

    for (const auto &destEntry : interfaces) {
        if ((destMask & destEntry.mask) == 0) {
            continue;
        }

        if (destEntry.iface == MIDI_INTERFACE_USB_DEVICE && !isConnectedToComputer) {
            continue;
        }

        if (destEntry.iface == MIDI_INTERFACE_USB_HOST && !midi_host_mounted) {
            continue;
        }

        if (isMidiDestFiltered(destEntry.iface, msg.type)) {
            continue;
        }

        forwardToInterface(destEntry.iface, msg);
    }

    if (source != MIDI_SOURCE_INTERNAL) {
        if (source == MIDI_SOURCE_SERIAL) {
            triggerSerialLED();
        } else {
            if (!(source == MIDI_SOURCE_USB_DEVICE && msg.type == MIDI_MSG_REALTIME && msg.rtType == midi::Clock)) {
                triggerUsbLED();
            }
        }
    }

    if (msg.type != MIDI_MSG_REALTIME || msg.rtType != midi::Clock) {
        dualPrintf("Router: src=%d type=%d ch=%d d1=%d d2=%d\r\n", source, msg.type, msg.channel, msg.data1, msg.data2);
    }
}

void routeMidiMessage(MidiSource source, const MidiMessage &msg) {
    byte destMask = ROUTE_TO_ALL;

    switch (source) {
        case MIDI_SOURCE_SERIAL:
            destMask &= ~ROUTE_TO_SERIAL;
            break;
        case MIDI_SOURCE_USB_DEVICE:
            destMask &= ~ROUTE_TO_USB_DEVICE;
            break;
        case MIDI_SOURCE_USB_HOST:
            destMask &= ~ROUTE_TO_USB_HOST;
            break;
        case MIDI_SOURCE_INTERNAL:
        default:
            break;
    }

    routeMidiMessage(source, msg, destMask);
}
