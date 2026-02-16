# Architecture Research

**Domain:** Embedded MIDI router with CV/Clock output (RP2040)
**Researched:** 2026-02-15
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
                         RP2040 Dual-Core System
 ===================================================================
                                                                    
  CORE 0 (Main)                          CORE 1 (USB Host)          
 ┌────────────────────────────────┐     ┌───────────────────────┐    
 │                                │     │                       │    
 │  ┌─────────────────────────┐   │     │  ┌─────────────────┐  │    
 │  │   MIDI Router Engine    │   │     │  │  USB Host MIDI   │  │    
 │  │   routeMidiMessage()    │◄──┼─────┼──│  Transport       │  │    
 │  │   (single entry point)  │   │FIFO │  │  (PIO USB)       │  │    
 │  └──────┬──────────────────┘   │     │  └─────────────────┘  │    
 │         │                      │     │                       │    
 │  ┌──────▼──────────────────┐   │     └───────────────────────┘    
 │  │    Filter Engine        │   │                                  
 │  │  channel + msg type     │   │                                  
 │  └──────┬──────────────────┘   │                                  
 │         │                      │                                  
 │  ┌──────▼──────────────────┐   │                                  
 │  │   Output Dispatcher     │   │                                  
 │  │                         │   │                                  
 │  │  ┌──────┐  ┌──────┐    │   │                                  
 │  │  │USB-D │  │Serial│    │   │                                  
 │  │  │ MIDI │  │ MIDI │    │   │                                  
 │  │  └──────┘  └──────┘    │   │                                  
 │  │  ┌──────┐  ┌──────┐    │   │                                  
 │  │  │ CV   │  │Clock │    │   │                                  
 │  │  │Output│  │Output│    │   │                                  
 │  │  └──────┘  └──────┘    │   │                                  
 │  └─────────────────────────┘   │                                  
 │                                │                                  
 │  ┌─────────┐  ┌─────────┐     │                                  
 │  │Config   │  │ LED     │     │                                  
 │  │Manager  │  │ Manager │     │                                  
 │  └─────────┘  └─────────┘     │                                  
 │  ┌─────────┐                   │                                  
 │  │  IMU    │                   │                                  
 │  │Handler  │                   │                                  
 │  └─────────┘                   │                                  
 └────────────────────────────────┘                                  
                                                                    
 ═══════════════════════════════════                                  
              Hardware                                               
 ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                                
 │EEPROM│ │ PWM  │ │ I2C  │ │ GPIO │                                
 │(Flash│ │(CV/  │ │(IMU) │ │(LEDs)│                                
 │ Emu) │ │Clock)│ │      │ │      │                                
 └──────┘ └──────┘ └──────┘ └──────┘                                
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **MIDI Router Engine** | Central message routing — receives from any input, dispatches to all enabled outputs | Single `routeMidiMessage()` function with source ID, message type, and data payload |
| **Filter Engine** | Decides which messages pass through per-interface, per-channel | 2D boolean array `[interface][msgType]` + 16-element channel mask (existing `midi_filters.cpp`) |
| **USB Host Transport** | Receive/send MIDI over PIO-based USB Host (Core 1) | TinyUSB host callbacks → packet parsing → enqueue to router (existing `usb_host_wrapper.cpp`) |
| **USB Device Transport** | Receive/send MIDI over native USB (Core 0) | Arduino MIDI Library with TinyUSB backend (existing `midi_instances.cpp`) |
| **Serial Transport** | Receive/send MIDI over hardware UART (Core 0) | Arduino MIDI Library over Serial1 (existing `serial_midi_handler.cpp`) |
| **CV Output** | Convert MIDI note/CC to PWM-based 1V/oct CV signal | PWM at high frequency → Sallen-Key LPF → OPA2335 gain stage → 0-5V on TRS tip |
| **Clock Output** | Convert MIDI Clock/Start/Stop to analog pulse train | GPIO pulse output → TRS ring |
| **Config Manager** | Persist/load settings to EEPROM with versioning | Struct-based EEPROM layout with magic bytes + version + CRC8 (replaces current sequential layout) |
| **Web Serial Config** | JSON command protocol over USB CDC for browser configuration | Deserialize commands, apply config, serialize state (existing `web_serial_config.cpp`) |
| **IMU Handler** | Read MPU6050 sensor, convert angles to MIDI CC | Complementary filter, rate-limited CC output (existing `imu_handler.cpp`) |
| **LED Manager** | Non-blocking activity indicators | State-machine LED patterns replacing blocking `blinkBothLEDs()` |

## Recommended Project Structure

```
rp2040/
├── rp2040.ino              # Entry point: setup/loop for both cores, minimal code
├── midi_router.h           # MidiMessage struct, routeMidiMessage() declaration
├── midi_router.cpp         # Generic routing engine (THE key refactor)
├── midi_types.h            # Shared enums: MidiInterfaceType, MidiMsgType, MidiMessage struct
├── midi_filters.h          # Filter API (existing, updated to use midi_types.h)
├── midi_filters.cpp        # Filter state management (existing, minimal changes)
├── midi_instances.h        # USB Device MIDI instance (existing)
├── midi_instances.cpp      # USB Device MIDI instance (existing)
├── usb_host_wrapper.h      # USB Host transport (existing, updated)
├── usb_host_wrapper.cpp    # USB Host transport (existing, updated to call router)
├── serial_midi_handler.h   # Serial transport (existing, simplified)
├── serial_midi_handler.cpp # Serial transport (existing, handlers replaced by router calls)
├── cv_output.h             # CV output module API
├── cv_output.cpp           # PWM-based CV output with 1V/oct mapping
├── clock_output.h          # Clock output module API
├── clock_output.cpp        # MIDI Clock → analog pulse conversion
├── config.h                # Config manager API (existing, updated for versioning)
├── config.cpp              # EEPROM with magic/version/CRC (existing, refactored)
├── web_serial_config.h     # Web Serial command handler (existing)
├── web_serial_config.cpp   # JSON protocol (existing, add new commands)
├── imu_handler.h           # IMU handler (existing)
├── imu_handler.cpp         # IMU handler (existing)
├── led_utils.h             # LED manager (existing, updated for non-blocking)
├── led_utils.cpp           # LED manager (existing, refactored)
├── serial_utils.h          # Debug utilities (existing, add #ifdef DEBUG)
├── serial_utils.cpp        # Debug utilities (existing)
├── version.h               # Firmware version (existing)
└── pin_config.h            # NEW: centralized GPIO pin assignments
```

### Structure Rationale

- **`midi_types.h`:** Shared type definitions extracted from `midi_filters.h` so that `midi_router.h`, `cv_output.h`, and `clock_output.h` can all reference `MidiMessage` without circular includes.
- **`midi_router.cpp`:** The single most important new file. Replaces ~530 lines of duplicated handlers with one `routeMidiMessage()` function. Every input source calls this one function.
- **`cv_output.cpp` / `clock_output.cpp`:** New output modules that plug into the router as additional output destinations. They don't know about USB/Serial — they receive routed messages through the same dispatch mechanism.
- **`pin_config.h`:** Centralizes all `#define` pin assignments currently scattered across `led_utils.h` (29, 19), `imu_handler.h` (26, 27), `rp2040.ino` (12), and `serial_midi_handler.cpp` (0, 1). Adding CV/Clock pins here prevents pin-conflict debugging nightmares.

## Architectural Patterns

### Pattern 1: Unified Message Struct + Generic Router

**What:** Define a single `MidiMessage` struct that captures any MIDI message regardless of type or source. All input handlers construct this struct and call one `routeMidiMessage()` function. The router checks filters, then dispatches to all non-source outputs.

**When to use:** Always. This is THE architecture change that eliminates 33+ duplicated handlers.

**Trade-offs:** Slight overhead from struct construction vs. direct function calls. Negligible on RP2040 at MIDI message rates (max ~3000 msgs/sec for MIDI Clock at 300 BPM).

**Example:**
```cpp
// midi_types.h
struct MidiMessage {
    MidiInterfaceType source;  // Who sent this message
    MidiMsgType       type;    // Note, CC, PitchBend, etc.
    uint8_t           channel; // 1-16 (0 for system messages)
    uint8_t           data1;   // Note number, CC number, etc.
    uint8_t           data2;   // Velocity, CC value, etc.
    int               extData; // For pitch bend (14-bit signed)
    uint8_t*          sysex;   // Pointer to SysEx data (NULL if not SysEx)
    unsigned          sysexLen;// SysEx data length
};

// midi_router.cpp
void routeMidiMessage(const MidiMessage& msg) {
    // 1. Channel filter (skip for system messages)
    if (msg.channel > 0 && !isChannelEnabled(msg.channel)) return;
    
    // 2. Source interface filter
    if (isMidiFiltered(msg.source, msg.type)) return;
    
    // 3. Forward to each destination (skip source to avoid echo)
    for (int dest = 0; dest < MIDI_INTERFACE_COUNT; dest++) {
        if (dest == msg.source) continue;
        if (isMidiFiltered((MidiInterfaceType)dest, msg.type)) continue;
        sendToInterface((MidiInterfaceType)dest, msg);
    }
    
    // 4. Feed CV/Clock outputs (these are NOT interfaces — they're outputs only)
    feedCVOutput(msg);
    feedClockOutput(msg);
    
    // 5. Trigger appropriate LED
    triggerActivityLED(msg.source);
}
```

### Pattern 2: Output-Only Modules (CV/Clock as Sinks, Not Interfaces)

**What:** CV and Clock outputs are message *sinks* — they consume MIDI data but never produce it. They should NOT be added to `MidiInterfaceType` enum. Instead, they subscribe to specific message types from the router output.

**When to use:** For any output that converts MIDI to a non-MIDI signal (CV voltage, clock pulse, LED visualization, etc.).

**Trade-offs:** CV/Clock can't be individually filtered using the existing interface filter matrix. Use separate config flags for CV/Clock enable/disable and source selection instead.

**Why not add as interfaces:** Adding CV/Clock to `MidiInterfaceType` would grow the filter matrix and imply they could be routing *sources* (they can't — there's no CV-to-MIDI input). It would also break the symmetry of the current "any source routes to any destination" model.

**Example:**
```cpp
// cv_output.h
struct CVConfig {
    bool      enabled;
    uint8_t   sourceChannel;    // Which MIDI channel drives CV (1-16, 0=omni)
    uint8_t   mode;             // CV_MODE_NOTE_PITCH or CV_MODE_CC
    uint8_t   ccNumber;         // CC number when in CC mode
    uint8_t   cvPin;            // GPIO pin for PWM output
    uint16_t  pwmRange;         // PWM resolution (e.g., 4096 for 12-bit)
    float     voltPerOctave;    // Calibration: volts per octave
};

void setupCVOutput();
void feedCVOutput(const MidiMessage& msg);  // Called by router
```

### Pattern 3: EEPROM Versioned Config Block

**What:** Store configuration as a versioned struct with magic bytes and integrity check, instead of sequential address-incrementing byte writes.

**When to use:** Any time EEPROM layout needs to support firmware upgrades without corrupting user settings.

**Trade-offs:** Uses a few extra bytes for header (~4 bytes), but RP2040 supports 4096 bytes of EEPROM emulation so this is negligible.

**Example:**
```cpp
// config.h
#define CONFIG_MAGIC_0   0x4D  // 'M'
#define CONFIG_MAGIC_1   0x50  // 'P'
#define CONFIG_VERSION   2     // Increment on layout change

struct __attribute__((packed)) ConfigBlock {
    // Header (4 bytes)
    uint8_t  magic[2];         // CONFIG_MAGIC_0, CONFIG_MAGIC_1
    uint8_t  version;          // CONFIG_VERSION
    uint8_t  checksum;         // CRC8 of everything after this byte
    
    // Payload (grows with features)
    bool     midiFilters[MIDI_INTERFACE_COUNT][MIDI_MSG_COUNT];  // 24 bytes
    bool     enabledChannels[16];                                 // 16 bytes
    IMUConfig imuConfig;                                          // ~40 bytes
    CVConfig  cvConfig;                                           // ~10 bytes
    ClockConfig clockConfig;                                      // ~4 bytes
    // Total: ~98 bytes + 4 header = ~102 bytes
};

void saveConfigToEEPROM() {
    ConfigBlock block;
    block.magic[0] = CONFIG_MAGIC_0;
    block.magic[1] = CONFIG_MAGIC_1;
    block.version = CONFIG_VERSION;
    // ... populate fields from runtime state ...
    block.checksum = crc8((uint8_t*)&block + 4, sizeof(block) - 4);
    
    EEPROM.begin(sizeof(ConfigBlock));
    EEPROM.put(0, block);
    EEPROM.commit();
    EEPROM.end();
}

void loadConfigFromEEPROM() {
    EEPROM.begin(sizeof(ConfigBlock));
    ConfigBlock block;
    EEPROM.get(0, block);
    EEPROM.end();
    
    // Validate
    if (block.magic[0] != CONFIG_MAGIC_0 || block.magic[1] != CONFIG_MAGIC_1) {
        applyDefaults();  // First boot or corrupted
        return;
    }
    uint8_t expected = crc8((uint8_t*)&block + 4, sizeof(block) - 4);
    if (block.checksum != expected) {
        applyDefaults();  // Corrupted data
        return;
    }
    if (block.version < CONFIG_VERSION) {
        migrateConfig(block);  // Handle version upgrades
        return;
    }
    // Apply valid config to runtime state
    applyConfig(block);
}
```

### Pattern 4: Non-Blocking State Machine for LED/Calibration

**What:** Replace all `delay()`-based blocking patterns with state machines driven by `millis()` checks in the main loop.

**When to use:** Any visual feedback (LED blinks, calibration sequences) that currently uses `delay()`.

**Trade-offs:** Slightly more complex code, but eliminates MIDI dropouts during LED animations and IMU calibration.

**Example:**
```cpp
// led_utils.cpp
enum BlinkState { BLINK_IDLE, BLINK_ON, BLINK_OFF };

static BlinkState blinkState = BLINK_IDLE;
static int blinksRemaining = 0;
static uint32_t blinkTimer = 0;
static uint32_t blinkInterval = 0;

void startBlinkPattern(int count, uint32_t intervalMs) {
    blinksRemaining = count;
    blinkInterval = intervalMs;
    blinkState = BLINK_ON;
    blinkTimer = millis();
    digitalWrite(LED_IN_PIN, HIGH);
    digitalWrite(LED_OUT_PIN, HIGH);
}

void handleLEDs() {
    // ... existing activity LED handling ...
    
    // Blink pattern state machine
    if (blinkState != BLINK_IDLE && millis() - blinkTimer >= blinkInterval) {
        blinkTimer = millis();
        if (blinkState == BLINK_ON) {
            digitalWrite(LED_IN_PIN, LOW);
            digitalWrite(LED_OUT_PIN, LOW);
            blinkState = BLINK_OFF;
        } else {
            blinksRemaining--;
            if (blinksRemaining <= 0) {
                blinkState = BLINK_IDLE;
            } else {
                digitalWrite(LED_IN_PIN, HIGH);
                digitalWrite(LED_OUT_PIN, HIGH);
                blinkState = BLINK_ON;
            }
        }
    }
}
```

## Data Flow

### MIDI Message Routing (Refactored)

```
 ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
 │  USB Host    │   │  USB Device  │   │  Serial MIDI │
 │  (Core 1)    │   │  (Core 0)    │   │  (Core 0)    │
 └──────┬───────┘   └──────┬───────┘   └──────┬───────┘
        │                  │                   │
        │  MidiMessage     │  MidiMessage      │  MidiMessage
        │  {source=HOST}   │  {source=DEVICE}  │  {source=SERIAL}
        │                  │                   │
        └──────────┬───────┴──────────┬────────┘
                   │                  │
                   ▼                  ▼
          ┌────────────────────────────────┐
          │       routeMidiMessage()       │
          │                                │
          │  1. Channel filter             │
          │  2. Source interface filter     │
          │  3. For each dest interface:   │
          │     - Skip if dest == source   │
          │     - Skip if dest filtered    │
          │     - sendToInterface(dest)    │
          │  4. feedCVOutput(msg)          │
          │  5. feedClockOutput(msg)       │
          │  6. triggerActivityLED(source) │
          └───┬──────┬──────┬──────┬──────┘
              │      │      │      │
              ▼      ▼      ▼      ▼
          ┌──────┐┌──────┐┌────┐┌─────┐
          │USB-D ││Serial││ CV ││Clock│
          │ Out  ││ Out  ││Out ││ Out │
          └──────┘└──────┘└────┘└─────┘
```

### CV Output Signal Path

```
  MidiMessage (NoteOn ch=1, note=60)
       │
       ▼
  feedCVOutput()
       │
       ├── Is CV enabled? No → return
       ├── Does channel match? No → return
       ├── Mode = NOTE_PITCH?
       │     │
       │     ▼
       │   noteToVoltage(note)
       │     │  C2 (36) = 0V, C7 (96) = 5V
       │     │  voltage = (note - 36) / 12.0
       │     │  5 octaves = 5V
       │     ▼
       │   voltageToPWM(voltage)
       │     │  PWM duty = voltage / 5.0 * pwmRange
       │     │  Accounts for gain stage (÷ 1.51)
       │     ▼
       │   analogWrite(cvPin, pwmDuty)
       │
       └── Mode = CC?
             │
             ▼
           ccToVoltage(value)  // 0-127 → 0-5V linear
             │
             ▼
           voltageToPWM(voltage)
             │
             ▼
           analogWrite(cvPin, pwmDuty)
```

### Clock Output Signal Path

```
  MidiMessage (Realtime: Clock/Start/Stop)
       │
       ▼
  feedClockOutput()
       │
       ├── msg == Start? → set running = true, reset counter
       ├── msg == Stop?  → set running = false, output LOW
       ├── msg == Clock?
       │     │
       │     ▼
       │   clockCounter++ (24 ppqn)
       │     │
       │     ├── counter % divisor == 0?
       │     │     │  divisor=1: 24ppqn, divisor=6: quarter note
       │     │     ▼
       │     │   digitalWrite(clockPin, HIGH)
       │     │   pulseStartTime = micros()
       │     │
       │     └── In handleClockOutput() (main loop):
       │           if (micros() - pulseStartTime > PULSE_WIDTH_US)
       │             digitalWrite(clockPin, LOW)
       │
       └── (pulse width typically 1-5ms for Eurorack compatibility)
```

### Configuration Flow (Updated)

```
  Browser Web Serial
       │
       ▼ JSON: {"command":"SAVEALL", "filters":[...], "cv":{...}, "clock":{...}}
  processWebSerialConfig()
       │
       ├── updateConfigFromJson() → applies to runtime state
       │     ├── midi filters
       │     ├── channel enables
       │     ├── IMU config
       │     ├── CV config     ← NEW
       │     └── Clock config  ← NEW
       │
       └── scheduleEEPROMSave() → 3s delayed write
             │
             ▼
           saveConfigToEEPROM()
             │
             ▼
           ConfigBlock with magic/version/CRC
```

### Key Data Flows

1. **MIDI routing:** Input transport → `MidiMessage` struct → `routeMidiMessage()` → filter check → `sendToInterface()` per destination + `feedCVOutput()` + `feedClockOutput()`
2. **CV pitch tracking:** NoteOn → note-to-voltage lookup → PWM duty cycle → hardware LPF → op-amp gain → 0-5V analog output on TRS tip
3. **Clock generation:** MIDI Clock (24ppqn) → counter with configurable divisor → GPIO pulse with fixed width → TRS ring
4. **Configuration:** Web Serial JSON → runtime state update → delayed EEPROM save with versioned block format
5. **Cross-core MIDI:** USB Host on Core 1 → construct `MidiMessage` → call `routeMidiMessage()` (runs on Core 1 context for USB Host source, Core 0 for others)

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Current (3 interfaces + CV/Clock) | Single `routeMidiMessage()` with interface enum. CV/Clock as output-only sinks. Fits in ~100 bytes EEPROM. |
| Adding 4th interface (e.g., BLE) | Add to `MidiInterfaceType` enum, filter matrix grows by one row. Router loop handles it automatically. No handler duplication needed. |
| Multiple CV outputs | `CVConfig` becomes an array. `feedCVOutput()` iterates. EEPROM grows but stays well under 4096 byte limit. |
| Complex routing rules (channel remap, transpose) | Add transform step between filter and dispatch in `routeMidiMessage()`. Message struct supports in-place modification. |

### Scaling Priorities

1. **First bottleneck: Debug logging.** At high MIDI throughput, `dualPrintf()` in the routing path adds latency. Fix with `#ifdef DEBUG` compile-time guards. This is the most impactful single change for performance.
2. **Second bottleneck: Cross-core data races.** `midi_host_mounted` and `midi_dev_addr` modified by Core 1, read by Core 0 without `volatile` or locking. Use `volatile` for simple flags, `rp2040.spin_lock` for multi-word state.

## Anti-Patterns

### Anti-Pattern 1: Per-Source-Per-Type Handler Functions

**What people do:** Write a separate `usbh_onNoteOnHandle()`, `usbd_onNoteOn()`, `localSerialOnNoteOn()` for every combination of source interface and message type. This is the current codebase pattern.
**Why it's wrong:** 33+ functions that all do the same thing: check channel → check source filter → log → forward to other interfaces → trigger LED. Any change requires updating all 33 functions identically. Bugs from inconsistency (e.g., missing channel filter on `usbd_onControlChange`) are inevitable.
**Do this instead:** One `routeMidiMessage(MidiMessage)` function. Input callbacks construct a `MidiMessage` struct and call the router. Each source's callback becomes 2-5 lines instead of 15-25.

### Anti-Pattern 2: Adding CV/Clock as MIDI Interfaces

**What people do:** Add `MIDI_INTERFACE_CV` and `MIDI_INTERFACE_CLOCK` to `MidiInterfaceType`, growing the filter matrix and handler matrix.
**Why it's wrong:** CV and Clock are output-only. They can never be a source of MIDI messages. Adding them to the interface enum implies bidirectional routing and wastes filter matrix space for impossible input filters.
**Do this instead:** Treat CV/Clock as output sinks that the router feeds after the interface-to-interface routing step. Give them their own enable/disable config separate from the MIDI filter matrix.

### Anti-Pattern 3: Sequential EEPROM Layout Without Header

**What people do:** Write config fields to EEPROM at incrementing addresses with no version marker or integrity check. This is the current codebase pattern.
**Why it's wrong:** Adding or removing any field shifts all subsequent addresses, silently corrupting saved configs from previous firmware versions. First boot on fresh hardware reads garbage. No way to detect corruption.
**Do this instead:** Use a packed struct with magic bytes at offset 0, a version byte at offset 2, and a CRC8 checksum at offset 3. On load, validate magic → validate CRC → check version → migrate if needed → apply. On invalid data, apply defaults.

### Anti-Pattern 4: Blocking Delays in the Main Loop

**What people do:** Use `delay()` for LED animations and IMU calibration, halting all MIDI processing. Current code blocks for 800ms on boot and 400ms on config save.
**Why it's wrong:** MIDI messages arriving during the delay are buffered (if the transport has a buffer) or dropped. At 120 BPM, 400ms = ~8 MIDI Clock messages lost. For a MIDI router, this is unacceptable in performance.
**Do this instead:** Use `millis()`-based state machines for all visual feedback. IMU calibration should be a background state machine that collects samples across multiple loop iterations.

## Integration Points

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Input transports → Router | Function call: `routeMidiMessage(msg)` | USB Host handlers on Core 1 call router directly. Thread safety via `volatile` flags for shared state (`midi_host_mounted`). No mutex needed — routing is stateless and read-only on shared config. |
| Router → Output transports | Function call: `sendToInterface(dest, msg)` | Switch on dest type, call appropriate send function. USB Host send goes through `sendMidiPacket()`. |
| Router → CV/Clock | Function call: `feedCVOutput(msg)`, `feedClockOutput(msg)` | One-way push. CV/Clock modules maintain their own state (last note, clock counter). |
| Config Manager → All modules | Function call: `applyConfig(block)` / `getModuleConfig()` | Config manager reads/writes module state via getter/setter functions. Modules own their runtime state. |
| Web Serial → Config Manager | JSON command → `processWebSerialConfig()` → `updateConfigFromJson()` → `saveConfigToEEPROM()` | Unchanged from current architecture. Add `cv` and `clock` JSON keys. |
| Core 0 ↔ Core 1 | Shared globals with `volatile`, startup sync via FIFO | `midi_host_mounted` and `midi_dev_addr` must be `volatile`. No mutex needed because USB Host packet processing is self-contained on Core 1 — it calls `routeMidiMessage()` which reads filter state (written only from Core 0 config path, effectively single-writer). |

### Hardware Integration

| Hardware | Interface | GPIO Pins | Notes |
|----------|-----------|-----------|-------|
| USB Host (PIO) | PIO USB bit-bang | GPIO 12 (D+), 13 (D-) | Core 1 exclusive. Must use PIO. |
| USB Device | Native USB | Internal | Shared with Web Serial CDC. |
| Serial MIDI | UART (Serial1) | GPIO 0 (TX), 1 (RX) | 31250 baud standard MIDI. |
| IMU (MPU6050) | I2C (Wire1) | GPIO 26 (SDA), 27 (SCL) | 400kHz. |
| Debug UART | UART (Serial2) | GPIO 24 (TX), 25 (RX) | 115200 baud. |
| LED (Serial activity) | GPIO output | GPIO 29 | Active HIGH. |
| LED (USB activity) | GPIO output | GPIO 19 | Active HIGH. |
| CV Output | PWM | **TBD** (suggest GPIO 2 or 3) | Must be on different PWM slice from Clock pin. Needs Sallen-Key LPF + OPA2335 gain on PCB. |
| Clock Output | GPIO | **TBD** (suggest GPIO 4 or 5) | Digital pulse, no filtering needed. Can share PWM slice with CV if on same slice's other channel, but GPIO toggle is simpler and adequate. |

### PWM Configuration for CV Output

The RP2040's PWM operates with the following constraints verified from official arduino-pico docs (HIGH confidence):

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| PWM Frequency | ~30-50 kHz | High enough for Sallen-Key filter (fc=9947Hz) to attenuate carrier. Too high reduces resolution. |
| PWM Resolution | ~12 bits (4096 steps) | At 120MHz clock, 30kHz PWM gives ~4000 counts. Sufficient for 5-octave CV (5V / 4096 = 1.2mV/step, 1V/oct needs ~8.3mV/semitone). |
| `analogWriteFreq()` | 30000 (30kHz) | Set before first `analogWrite()` call. |
| `analogWriteRange()` | 4096 | 12-bit resolution. Maps 0-4095 to 0-100% duty. |
| Voltage mapping | PWM duty → 3.3V → ×1.51 gain → 0-4.98V | OPA2335 non-inverting amp. Full-scale PWM (3.3V) × 1.51 = 4.98V ≈ 5V. |
| Note range | C2 (36) to C7 (96) = 5 octaves | 60 semitones across 5V. Each semitone = 83.3mV = ~103 PWM steps. Adequate precision. |

## Build Order (Dependencies)

The following build order respects component dependencies and minimizes risk:

```
Phase 1: Foundation (no new features, pure refactor)
 ├── Extract midi_types.h (shared enums/structs)
 ├── Create midi_router.cpp with routeMidiMessage()
 ├── Replace all 33+ handlers with router calls
 ├── Centralize pin_config.h
 └── Verify: all existing MIDI routing still works identically

Phase 2: Config Hardening (enables safe evolution)
 ├── Implement ConfigBlock struct with magic/version/CRC
 ├── Add factory reset mechanism
 ├── Migrate existing EEPROM data on load
 └── Verify: config survives firmware update

Phase 3: Code Quality (reduces bug surface)
 ├── Add #ifdef DEBUG compile-time guards
 ├── Add volatile to cross-core shared vars
 ├── Replace blocking delays with state machines
 ├── Fix known bugs (SysEx truncation, missing filters)
 └── Verify: no MIDI dropouts during LED blinks

Phase 4: CV Output (new hardware feature)
 ├── Implement cv_output.cpp with PWM setup
 ├── Add feedCVOutput() call in router
 ├── Add CVConfig to ConfigBlock
 ├── Add CV config to Web Serial protocol + UI
 └── Verify: 1V/oct accuracy with scope/multimeter

Phase 5: Clock Output (new hardware feature)
 ├── Implement clock_output.cpp with GPIO pulse
 ├── Add feedClockOutput() call in router
 ├── Add ClockConfig to ConfigBlock
 ├── Add clock config to Web Serial protocol + UI
 └── Verify: clock tracks MIDI tempo correctly
```

**Ordering rationale:**
- Phase 1 before everything else: the generic router makes all subsequent changes simpler. Adding CV/Clock to 33 duplicated handlers would be madness.
- Phase 2 before Phase 4/5: CV/Clock config needs to be stored in EEPROM. The versioned config block must exist before adding new config fields.
- Phase 3 can partially overlap with Phase 2. Debug guards and volatile fixes are independent of config refactoring.
- Phase 4 before Phase 5: CV output is the more complex feature (PWM configuration, voltage calibration, note mapping). Clock is simpler (GPIO toggle). CV validates the output-sink architecture pattern.

## Sources

- Codebase analysis: `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/CONCERNS.md`, `.planning/codebase/STRUCTURE.md` (2026-02-15)
- Full source code review: all 20 files in `rp2040/` directory (read in this session)
- Arduino-Pico PWM documentation: https://arduino-pico.readthedocs.io/en/latest/analog.html (verified 2026-02-15, HIGH confidence)
- RP2040 PWM constraints: arduino-pico docs confirm 16-bit max range, frequency/resolution trade-off, `analogWriteFreq()`/`analogWriteRange()` API (HIGH confidence)
- MIDI specification: 24 ppqn clock, standard message types, 31250 baud serial (domain knowledge, HIGH confidence)
- 1V/oct CV standard: 1 volt per octave, typically 0-5V or 0-10V range (domain knowledge, HIGH confidence)
- PROJECT.md hardware specs: OPA2335AIDR, Sallen-Key fc=9947Hz, gain=1.51x (from project context, HIGH confidence)

---
*Architecture research for: MIDI PicoLink — RP2040 MIDI Router with CV/Clock Output*
*Researched: 2026-02-15*
