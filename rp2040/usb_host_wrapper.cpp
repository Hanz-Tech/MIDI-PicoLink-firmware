#if defined(USE_TINYUSB_HOST) || !defined(USE_TINYUSB)
#error "Please use the Menu to select Tools->USB Stack: Adafruit TinyUSB"
#endif
#include "usb_host_wrapper.h"
#include "usb_host_midi_handlers.h"
#include "serial_utils.h"
#include "led_utils.h"
#include <MIDI.h>
#include "pico/sync.h"

// MIDI host state variables
volatile uint8_t midi_dev_addr = 0;
uint8_t midi_dev_idx = 0;
volatile bool midi_host_mounted = false;

auto_init_mutex(midi_host_mutex);

// Add general USB host callbacks for debugging
void tuh_mount_cb(uint8_t daddr) {
    dualPrintf("USB Host: Device mounted at address %u\r\n", daddr);
    triggerUsbLED();
}

void tuh_umount_cb(uint8_t daddr) {
    dualPrintf("USB Host: Device unmounted at address %u\r\n", daddr);
    triggerUsbLED();
}

// Add configuration callback for more detailed debugging
bool tuh_configuration_set_cb(uint8_t daddr, uint8_t config_num) {
    dualPrintf("USB Host: Configuration %u set for device at address %u\r\n", config_num, daddr);
    return true; // Allow configuration to proceed
}

// TinyUSB MIDI host callback implementations
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data) {
    dualPrintf("USB Host: MIDI device mounted at idx %u with device addr %u\r\n", 
               idx, mount_cb_data->daddr);
    triggerUsbLED();
    
    midi_dev_idx = idx;
    mutex_enter_blocking(&midi_host_mutex);
    midi_dev_addr = mount_cb_data->daddr;
    midi_host_mounted = true;
    mutex_exit(&midi_host_mutex);
    
    // Call application callback with estimated cable counts
    onMIDIconnect(midi_dev_addr, 1, 1); // Default to 1 IN and 1 OUT cable
}

void tuh_midi_umount_cb(uint8_t idx) {
    dualPrintf("MIDI device at idx %u unmounted\r\n", idx);
    triggerUsbLED();
    
    if (midi_dev_idx == idx) {
        // Call application callback before clearing state
        onMIDIdisconnect(midi_dev_addr);
        
        midi_dev_idx = 0;
        mutex_enter_blocking(&midi_host_mutex);
        midi_dev_addr = 0;
        midi_host_mounted = false;
        mutex_exit(&midi_host_mutex);
    }
}

bool getMidiHostState(uint8_t *addr) {
    bool mounted = false;
    uint8_t current_addr = 0;

    mutex_enter_blocking(&midi_host_mutex);
    mounted = midi_host_mounted;
    current_addr = midi_dev_addr;
    mutex_exit(&midi_host_mutex);

    if (addr != nullptr) {
        *addr = current_addr;
    }

    return mounted;
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t xferred_bytes) {    
    // Read MIDI packets
    uint8_t packet[4];
    
    // Process all available packets
    while (tuh_midi_packet_read(idx, packet)) {
        triggerUsbLED();
        processMidiPacket(packet);
    }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t xferred_bytes) {
    // TX callback - could be used for flow control if needed
    (void)idx;
    (void)xferred_bytes;
}

// Process a received MIDI packet and convert to MIDI library format
void processMidiPacket(uint8_t packet[4]) {
    uint8_t cable = (packet[0] >> 4) & 0x0F;
    uint8_t cin = packet[0] & 0x0F;
    uint8_t msg[3] = {packet[1], packet[2], packet[3]};
        
    // Ignore invalid packets
    if (cin == 0) return;
    
    // Extract message type and channel
    uint8_t status = msg[0];
    uint8_t channel = (status & 0x0F) + 1; // MIDI channels are 1-16
    uint8_t msgType = status & 0xF0;
    
    // Process based on message type
    switch (msgType) {
        case 0x80: // Note Off
            if (cin == 0x8) {
                usbh_onNoteOffHandle(channel, msg[1], msg[2]);
            }
            break;
            
        case 0x90: // Note On
            if (cin == 0x9) {
                usbh_onNoteOnHandle(channel, msg[1], msg[2]);
            }
            break;
            
        case 0xA0: // Polyphonic Aftertouch
            if (cin == 0xA) {
                usbh_onPolyphonicAftertouchHandle(channel, msg[1], msg[2]);
            }
            break;
            
        case 0xB0: // Control Change
            if (cin == 0xB) {
                usbh_onControlChangeHandle(channel, msg[1], msg[2]);
            }
            break;
            
        case 0xC0: // Program Change
            if (cin == 0xC) {
                usbh_onProgramChangeHandle(channel, msg[1]);
            }
            break;
            
        case 0xD0: // Channel Aftertouch
            if (cin == 0xD) {
                usbh_onAftertouchHandle(channel, msg[1]);
            }
            break;
            
        case 0xE0: // Pitch Bend
            if (cin == 0xE) {
                int bend = (msg[2] << 7) | msg[1];
                bend -= 8192; // Convert to signed value (-8192 to +8191)
                usbh_onPitchBendHandle(channel, bend);
            }
            break;
            
        case 0xF0: // System messages
            switch (status) {
                case 0xF8: // MIDI Clock
                    if (cin == 0xF) {
                        usbh_onMidiClockHandle();
                    }
                    break;
                case 0xFA: // Start
                    if (cin == 0xF) {
                        usbh_onMidiStartHandle();
                    }
                    break;
                case 0xFB: // Continue
                    if (cin == 0xF) {
                        usbh_onMidiContinueHandle();
                    }
                    break;
                case 0xFC: // Stop
                    if (cin == 0xF) {
                        usbh_onMidiStopHandle();
                    }
                    break;
                case 0xF0: // SysEx start - handle multi-packet SysEx
                    // For simplicity, we'll handle single-packet SysEx for now
                    // Full SysEx implementation would require buffering across packets
                    if (cin == 0x4 || cin == 0x5 || cin == 0x6 || cin == 0x7) {
                        // Single packet SysEx or SysEx end
                        uint8_t sysex_data[3];
                        uint8_t sysex_len = 0;
                        
                        if (cin == 0x4) sysex_len = 3; // 3-byte SysEx
                        else if (cin == 0x5) sysex_len = 1; // 1-byte SysEx end
                        else if (cin == 0x6) sysex_len = 2; // 2-byte SysEx end
                        else if (cin == 0x7) sysex_len = 3; // 3-byte SysEx end
                        
                        for (int i = 0; i < sysex_len; i++) {
                            sysex_data[i] = msg[i];
                        }
                        
                        usbh_onSysExHandle(sysex_data, sysex_len);
                    }
                    break;
            }
            break;
    }
}

// Send a MIDI packet to the host device
bool sendMidiPacket(uint8_t packet[4]) {
    if (!midi_host_mounted) return false;

    if (!tuh_midi_packet_write(midi_dev_idx, packet)) {
        return false;
    }

    tuh_midi_write_flush(midi_dev_idx);
    return true;
}

// Helper functions to send specific MIDI messages
bool sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t packet[4] = {0x09, (uint8_t)(0x90 | (channel - 1)), note, velocity};
    return sendMidiPacket(packet);
}

bool sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t packet[4] = {0x08, (uint8_t)(0x80 | (channel - 1)), note, velocity};
    return sendMidiPacket(packet);
}

bool sendControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
    uint8_t packet[4] = {0x0B, (uint8_t)(0xB0 | (channel - 1)), controller, value};
    return sendMidiPacket(packet);
}

bool sendProgramChange(uint8_t channel, uint8_t program) {
    uint8_t packet[4] = {0x0C, (uint8_t)(0xC0 | (channel - 1)), program, 0};
    return sendMidiPacket(packet);
}

bool sendAfterTouch(uint8_t channel, uint8_t pressure) {
    uint8_t packet[4] = {0x0D, (uint8_t)(0xD0 | (channel - 1)), pressure, 0};
    return sendMidiPacket(packet);
}

bool sendPolyAfterTouch(uint8_t channel, uint8_t note, uint8_t pressure) {
    uint8_t packet[4] = {0x0A, (uint8_t)(0xA0 | (channel - 1)), note, pressure};
    return sendMidiPacket(packet);
}

bool sendPitchBend(uint8_t channel, int bend) {
    bend += 8192; // Convert to unsigned 14-bit value
    uint8_t lsb = bend & 0x7F;
    uint8_t msb = (bend >> 7) & 0x7F;
    uint8_t packet[4] = {0x0E, (uint8_t)(0xE0 | (channel - 1)), lsb, msb};
    return sendMidiPacket(packet);
}

bool sendRealTime(uint8_t rtByte) {
    uint8_t packet[4] = {0x0F, rtByte, 0, 0};
    return sendMidiPacket(packet);
}

bool sendSysEx(unsigned size, byte* array) {
    if (!midi_host_mounted || size == 0) return false;
    
    // For simplicity, we'll handle small SysEx messages in a single packet
    // Full implementation would need to split larger SysEx across multiple packets
    if (size <= 3) {
        uint8_t packet[4] = {0, 0, 0, 0};
        
        if (size == 1) packet[0] = 0x05; // 1-byte SysEx
        else if (size == 2) packet[0] = 0x06; // 2-byte SysEx  
        else if (size == 3) packet[0] = 0x07; // 3-byte SysEx
        
        for (unsigned i = 0; i < size; i++) {
            packet[i + 1] = array[i];
        }
        
        return sendMidiPacket(packet);
    }
    
    // For larger SysEx, we'd need to implement multi-packet transmission
    // This is a simplified version for now
    return false;
}

// MIDI event handlers (same as before)
void onActiveSense() {
    dualPrintf("ASen\r\n");
}

void onSystemReset() {
    dualPrintf("SysRst\r\n");
}

void skip() {}

void onMidiInWriteFail(uint8_t devAddr, uint8_t cable, bool fifoOverflow) {
    if (fifoOverflow)
        dualPrintf("Dev %u cable %u: MIDI IN FIFO overflow\r\n", devAddr, cable);
    else
        dualPrintf("Dev %u cable %u: MIDI IN FIFO error\r\n", devAddr, cable);
}

void onMidiError(int8_t errCode) {
    dualPrintf("MIDI Errors: %s %s %s\r\n", (errCode & (1UL << 0)) ? "Parse":"",
        (errCode & (1UL << 1)) ? "Active Sensing Timeout" : "",
        (errCode & (1UL << 2)) ? "Split SysEx":"");
}

void onSMPTEqf(byte data) {
    uint8_t type = (data >> 4) & 0xF;
    data &= 0xF;    
    static const char* fps[4] = {"24", "25", "30DF", "30ND"};
    switch (type) {
        case 0: dualPrintf("SMPTE FRM LS %u \r\n", data); break;
        case 1: dualPrintf("SMPTE FRM MS %u \r\n", data); break;
        case 2: dualPrintf("SMPTE SEC LS %u \r\n", data); break;
        case 3: dualPrintf("SMPTE SEC MS %u \r\n", data); break;
        case 4: dualPrintf("SMPTE MIN LS %u \r\n", data); break;
        case 5: dualPrintf("SMPTE MIN MS %u \r\n", data); break;
        case 6: dualPrintf("SMPTE HR LS %u \r\n", data); break;
        case 7:
            dualPrintf("SMPTE HR MS %u FPS:%s\r\n", data & 0x1, fps[(data >> 1) & 3]);
            break;
        default:
          dualPrintf("invalid SMPTE data byte %u\r\n", data);
          break;
    }
}

void onSongPosition(unsigned beats) {
    dualPrintf("SongP=%u\r\n", beats);
}

void onTuneRequest() {
    dualPrintf("Tune\r\n");
}

void onSongSelect(byte songnumber) {
    dualPrintf("SongS#%u\r\n", songnumber);
}

void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables) {
    dualPrintf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
    mutex_enter_blocking(&midi_host_mutex);
    midi_dev_addr = devAddr;
    mutex_exit(&midi_host_mutex);
}

void onMIDIdisconnect(uint8_t devAddr) {
    dualPrintf("MIDI device at address %u unplugged\r\n", devAddr);
    mutex_enter_blocking(&midi_host_mutex);
    if (midi_dev_addr == devAddr) {
        midi_dev_addr = 0;
    }
    mutex_exit(&midi_host_mutex);
}

void usb_host_wrapper_task() {
    // Debug: Show that the task is running periodically
    static uint32_t last_debug = 0;
    static bool first_run = true;
    uint32_t now = millis();
    
    if (first_run) {
        dualPrintln("USB Host wrapper task started");
        first_run = false;
    }
    
    // if (now - last_debug > 5000) { // Every 5 seconds
    //     dualPrintf("USB Host: Task running, MIDI mounted: %s\r\n", 
    //                midi_host_mounted ? "YES" : "NO");
        
    //     // Check for any connected devices
    //     bool any_device_connected = false;
    //     for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
    //         if (tuh_mounted(daddr)) {
    //             any_device_connected = true;
    //             dualPrintf("USB Host: Device confirmed at address %u\r\n", daddr);
    //         }
    //     }
        
    //     if (!any_device_connected) {
    //         dualPrintf("USB Host: No devices currently detected\r\n");
    //     }
        
    //     last_debug = now;
    // }
    
    // This function should be called in the main loop
    USBHost.task();
    // MIDI processing is now handled automatically by TinyUSB callbacks
}
