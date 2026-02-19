
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

#define LANGUAGE_ID 0x0409  // English

// MIDI host state
extern volatile uint8_t midi_dev_addr;
extern uint8_t midi_dev_idx;
extern volatile bool midi_host_mounted;
extern Adafruit_USBH_Host USBHost;

// TinyUSB MIDI host callback functions
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data);
void tuh_midi_umount_cb(uint8_t idx);
void tuh_midi_rx_cb(uint8_t idx, uint32_t xferred_bytes);
void tuh_midi_tx_cb(uint8_t idx, uint32_t xferred_bytes);

// Application callbacks
void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables);
void onMIDIdisconnect(uint8_t devAddr);

// MIDI handler function declarations (implemented in main sketch)
void usbh_onNoteOnHandle(byte channel, byte note, byte velocity);
void usbh_onNoteOffHandle(byte channel, byte note, byte velocity);
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

// MIDI packet processing
void processMidiPacket(uint8_t packet[4]);
bool sendMidiPacket(uint8_t packet[4]);

// Helper functions to send specific MIDI messages
bool sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
bool sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
bool sendControlChange(uint8_t channel, uint8_t controller, uint8_t value);
bool sendProgramChange(uint8_t channel, uint8_t program);
bool sendAfterTouch(uint8_t channel, uint8_t pressure);
bool sendPolyAfterTouch(uint8_t channel, uint8_t note, uint8_t pressure);
bool sendPitchBend(uint8_t channel, int bend);
bool sendSysEx(unsigned size, byte* array);
bool sendRealTime(uint8_t rtByte);

// Task functions
void usb_host_wrapper_task();

// Thread-safe accessor for host state (returns mounted, writes addr)
bool getMidiHostState(uint8_t *addr);
#endif
