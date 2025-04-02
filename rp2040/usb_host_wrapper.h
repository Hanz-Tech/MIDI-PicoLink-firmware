
/*!
 *  @file       usb_host.h
 *  Project     Pocket Operator MIDI Adapter v5
 *  @brief      Pocket Operator MIDI Adapter v5
 *  @author     Hanz Tech Inc
 *  @date       2024/06/17
 *  @license    MIT - Copyright (c) 2024 Hanz Tech Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef USB_HOST_WRAPPER_H
#define USB_HOST_WRAPPER_H

#include <Adafruit_TinyUSB.h>
#include "pio_usb.h"
#include "EZ_USB_MIDI_HOST.h"

USING_NAMESPACE_MIDI
USING_NAMESPACE_EZ_USB_MIDI_HOST

#define LANGUAGE_ID 0x0409  // English


USING_NAMESPACE_MIDI
USING_NAMESPACE_EZ_USB_MIDI_HOST

#define LANGUAGE_ID 0x0409  // English

extern EZ_USB_MIDI_HOST<MidiHostSettingsDefault>& midiHost;

void initializeMidiHost(EZ_USB_MIDI_HOST<MidiHostSettingsDefault>& midiHostInstance);
void registerMidiInCallbacks();
void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables);
void onMIDIdisconnect(uint8_t devAddr);
typedef void (*NoteOffFunctionPtr)(byte channel, byte note, byte velocity);
typedef void (*NoteOnFunctionPtr)(byte channel, byte note, byte velocity);
typedef void (*PolyphonicAftertouchFunctionPtr)(byte channel, byte note, byte pressure);
typedef void (*ControlChangeFunctionPtr)(byte channel, byte control, byte value);
typedef void (*ProgramChangeFunctionPtr)(byte channel, byte program);
typedef void (*AftertouchFunctionPtr)(byte channel, byte pressure);
typedef void (*PitchBendFunctionPtr)(byte channel, int bend);
typedef void (*SysExFunctionPtr)(byte * array, unsigned size);
typedef void (*MidiClockFunctionPtr)(void);
typedef void (*MidiStartFunctionPtr)(void);
typedef void (*MidiContinueFunctionPtr)(void);
typedef void (*MidiStopFunctionPtr)(void);

extern NoteOffFunctionPtr onNoteOff;
extern NoteOnFunctionPtr onNoteOn;
extern PolyphonicAftertouchFunctionPtr onPolyphonicAftertouch;
extern ControlChangeFunctionPtr onControlChange;
extern ProgramChangeFunctionPtr onProgramChange;
extern AftertouchFunctionPtr onAftertouch;
extern PitchBendFunctionPtr onPitchBend;
extern SysExFunctionPtr onSysEx;
extern MidiClockFunctionPtr onMidiClock;
extern MidiStartFunctionPtr onMidiStart;
extern MidiContinueFunctionPtr onMidiContinue;
extern MidiStopFunctionPtr onMidiStop;

#endif