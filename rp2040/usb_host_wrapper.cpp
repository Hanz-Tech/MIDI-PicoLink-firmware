

#if defined(USE_TINYUSB_HOST) || !defined(USE_TINYUSB)
#error "Please use the Menu to select Tools->USB Stack: Adafruit TinyUSB"
#endif
#include "usb_host_wrapper.h"
#define HOST_PIN_DP   16   // Pin used as D+ for host, D- = D+ + 1

uint8_t midi_dev_addr = 0;

void onActiveSense(){
    Serial.printf("ASen\r\n");
}

void onSystemReset(){
    Serial.printf("SysRst\r\n");
}

void skip(){}

void onMidiInWriteFail(uint8_t devAddr, uint8_t cable, bool fifoOverflow){
    if (fifoOverflow)
        Serial.printf("Dev %u cable %u: MIDI IN FIFO overflow\r\n", devAddr, cable);
    else
        Serial.printf("Dev %u cable %u: MIDI IN FIFO error\r\n", devAddr, cable);
}

void onMidiError(int8_t errCode){
    Serial.printf("MIDI Errors: %s %s %s\r\n", (errCode & (1UL << ErrorParse)) ? "Parse":"",
        (errCode & (1UL << ErrorActiveSensingTimeout)) ? "Active Sensing Timeout" : "",
        (errCode & (1UL << WarningSplitSysEx)) ? "Split SysEx":"");
}

void onSMPTEqf(byte data){
    uint8_t type = (data >> 4) & 0xF;
    data &= 0xF;    
    static const char* fps[4] = {"24", "25", "30DF", "30ND"};
    switch (type) {
        case 0: Serial.printf("SMPTE FRM LS %u \r\n", data); break;
        case 1: Serial.printf("SMPTE FRM MS %u \r\n", data); break;
        case 2: Serial.printf("SMPTE SEC LS %u \r\n", data); break;
        case 3: Serial.printf("SMPTE SEC MS %u \r\n", data); break;
        case 4: Serial.printf("SMPTE MIN LS %u \r\n", data); break;
        case 5: Serial.printf("SMPTE MIN MS %u \r\n", data); break;
        case 6: Serial.printf("SMPTE HR LS %u \r\n", data); break;
        case 7:
            Serial.printf("SMPTE HR MS %u FPS:%s\r\n", data & 0x1, fps[(data >> 1) & 3]);
            break;
        default:
          Serial.printf("invalid SMPTE data byte %u\r\n", data);
          break;
    }
}

void onSongPosition(unsigned beats){
    Serial.printf("SongP=%u\r\n", beats);
}

void onTuneRequest(){
    Serial.printf("Tune\r\n");
}


void onSongSelect(byte songnumber)
{
    Serial.printf("SongS#%u\r\n", songnumber);
}

void registerMidiInCallbacks() {
    auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, 0);
    if (intf == nullptr) return;

    intf->setHandleNoteOff(onNoteOff);
    intf->setHandleNoteOn(onNoteOn);
    intf->setHandleAfterTouchPoly(onPolyphonicAftertouch);
    intf->setHandleControlChange(onControlChange);
    intf->setHandleProgramChange(onProgramChange);
    intf->setHandleAfterTouchChannel(onAftertouch);
    intf->setHandlePitchBend(onPitchBend);
    intf->setHandleSystemExclusive(onSysEx);
    intf->setHandleTimeCodeQuarterFrame(onSMPTEqf);
    intf->setHandleSongPosition(onSongPosition);
    intf->setHandleSongSelect(onSongSelect);
    intf->setHandleTuneRequest(onTuneRequest);
    intf->setHandleClock(onMidiClock);
    intf->setHandleTick(skip);
    intf->setHandleStart(onMidiStart);
    intf->setHandleContinue(onMidiContinue);
    intf->setHandleStop(onMidiStop);
    intf->setHandleActiveSensing(onActiveSense);
    intf->setHandleSystemReset(onSystemReset);
    intf->setHandleError(onMidiError);

    auto dev = midiHost.getDevFromDevAddr(midi_dev_addr);
    if (dev == nullptr) return;
    dev->setOnMidiInWriteFail(onMidiInWriteFail);
}

void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables) {
    Serial.printf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
    midi_dev_addr = devAddr;
    registerMidiInCallbacks();
}

void onMIDIdisconnect(uint8_t devAddr)
{
    Serial.printf("MIDI device at address %u unplugged\r\n", devAddr);
    midi_dev_addr = 0;
}

