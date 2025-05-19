

#if defined(USE_TINYUSB_HOST) || !defined(USE_TINYUSB)
#error "Please use the Menu to select Tools->USB Stack: Adafruit TinyUSB"
#endif
#include "usb_host_wrapper.h"
#include "serial_utils.h"

uint8_t midi_dev_addr = 0;

void onActiveSense(){
    dualPrintf("ASen\r\n");
}

void onSystemReset(){
    dualPrintf("SysRst\r\n");
}

void skip(){}

void onMidiInWriteFail(uint8_t devAddr, uint8_t cable, bool fifoOverflow){
    if (fifoOverflow)
        dualPrintf("Dev %u cable %u: MIDI IN FIFO overflow\r\n", devAddr, cable);
    else
        dualPrintf("Dev %u cable %u: MIDI IN FIFO error\r\n", devAddr, cable);
}

void onMidiError(int8_t errCode){
    dualPrintf("MIDI Errors: %s %s %s\r\n", (errCode & (1UL << ErrorParse)) ? "Parse":"",
        (errCode & (1UL << ErrorActiveSensingTimeout)) ? "Active Sensing Timeout" : "",
        (errCode & (1UL << WarningSplitSysEx)) ? "Split SysEx":"");
}

void onSMPTEqf(byte data){
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

void onSongPosition(unsigned beats){
    dualPrintf("SongP=%u\r\n", beats);
}

void onTuneRequest(){
    dualPrintf("Tune\r\n");
}


void onSongSelect(byte songnumber)
{
    dualPrintf("SongS#%u\r\n", songnumber);
}

void registerMidiInCallbacks() {
    // Register callbacks for all in-cables of the device
    uint8_t ncables = midiHost.getNumInCables(midi_dev_addr);
    dualPrintf("Registering callbacks for %d in-cables on device address %d\r\n", ncables, midi_dev_addr);
    
    for (uint8_t cable = 0; cable < ncables; cable++) {
        auto intf = midiHost.getInterfaceFromDeviceAndCable(midi_dev_addr, cable);
        if (intf == nullptr) {
            dualPrintf("Failed to get interface for device %d, cable %d\r\n", midi_dev_addr, cable);
            continue;
        }

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
        
        dualPrintf("Successfully registered callbacks for device %d, cable %d\r\n", midi_dev_addr, cable);
    }

    auto dev = midiHost.getDevFromDevAddr(midi_dev_addr);
    if (dev == nullptr) return;
    dev->setOnMidiInWriteFail(onMidiInWriteFail);
}

void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables) {
    dualPrintf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
    midi_dev_addr = devAddr;
    registerMidiInCallbacks();
}

void unregisterMidiInCallbacks(uint8_t midiDevAddr) {
    uint8_t ncables = midiHost.getNumInCables(midiDevAddr);
    for (uint8_t cable = 0; cable < ncables; cable++) {
        auto intf = midiHost.getInterfaceFromDeviceAndCable(midiDevAddr, cable);
        if (intf == nullptr)
            continue;
        
        intf->disconnectCallbackFromType(NoteOn);
        intf->disconnectCallbackFromType(NoteOff);
        intf->disconnectCallbackFromType(AfterTouchPoly);
        intf->disconnectCallbackFromType(ControlChange);
        intf->disconnectCallbackFromType(ProgramChange);
        intf->disconnectCallbackFromType(AfterTouchChannel);
        intf->disconnectCallbackFromType(PitchBend);
        intf->disconnectCallbackFromType(SystemExclusive);
        intf->disconnectCallbackFromType(TimeCodeQuarterFrame);
        intf->disconnectCallbackFromType(SongPosition);
        intf->disconnectCallbackFromType(SongSelect);
        intf->disconnectCallbackFromType(TuneRequest);
        intf->disconnectCallbackFromType(Clock);
        intf->disconnectCallbackFromType(Tick);
        intf->disconnectCallbackFromType(Start);
        intf->disconnectCallbackFromType(Continue);
        intf->disconnectCallbackFromType(Stop);
        intf->disconnectCallbackFromType(ActiveSensing);
        intf->disconnectCallbackFromType(SystemReset);
        intf->setHandleError(nullptr);
    }
    
    auto dev = midiHost.getDevFromDevAddr(midiDevAddr);
    if (dev == nullptr)
        return;
    dev->setOnMidiInWriteFail(nullptr);
}

void onMIDIdisconnect(uint8_t devAddr)
{
    dualPrintf("MIDI device at address %u unplugged\r\n", devAddr);
    unregisterMidiInCallbacks(devAddr);
    midi_dev_addr = 0;
}
