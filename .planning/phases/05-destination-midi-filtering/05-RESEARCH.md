# Phase 05: Destination MIDI Filtering - Research

**Researched:** 2026-02-25
**Domain:** Embedded MIDI routing filters + Web Serial config
**Confidence:** MEDIUM

## User Constraints

- None found (no CONTEXT.md present).

## Summary

The current firmware uses a single 2D filter matrix in `rp2040/midi_filters.cpp` for per-interface message-type filtering, but it is only applied at the source stage in `rp2040/midi_router.cpp`. There is no destination-stage filter check today, so Phase 5 requires introducing a destination filter matrix and applying it inside the router loop before forwarding to each output. This must preserve existing source-side filtering semantics and keep the per-interface channel filter unchanged.

The configuration pipeline is JSON-based (Web Serial READALL/SAVEALL), stored in EEPROM as sequential bytes. Adding a destination filter matrix increases stored filter bytes from 3x8 to 6x8 if stored separately (source + destination). The Web UI (TypeScript + HTML) currently assumes a 3x8 filters array and uses inverted checkbox logic (checked = allowed, stored value = blocked). Both JSON validation and UI layout must expand to represent destination filters while preserving existing source filters.

**Primary recommendation:** Add a distinct destination filter matrix with explicit API (get/set/isDestinationFiltered), apply it in `routeMidiMessage()` per destination before forwarding, and extend config JSON/UI to include both source and destination filters without changing the meaning of existing fields.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Arduino (arduino-pico) | 5.5.0 (repo baseline) | RP2040 firmware platform | Already used in firmware; no new dependencies needed |
| ArduinoJson | (repo dependency) | JSON parsing for Web Serial config | Existing config protocol uses it |
| AJV | (web_configurator/package.json) | JSON schema validation in UI | Existing UI validation tool |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| TinyUSB (Arduino core) | bundled | USB Device/Host transport | Already used by MIDI handlers |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| New filter storage layer | Reuse existing matrix for both source/destination | Conflates semantics; harder to preserve existing behavior |

**Installation:**
```bash
# No new packages required
```

## Architecture Patterns

### Recommended Project Structure
```
rp2040/
├── midi_filters.h          # Filter API (extend with destination filter APIs)
├── midi_filters.cpp        # Filter storage (add destination matrix)
├── midi_router.cpp         # Apply source + destination filters in routing loop
├── config.cpp              # JSON/EEPROM (extend for destination filters)
web_configurator/
├── src/app.ts              # UI for source + destination filter matrices
├── src/validator.ts        # JSON schema updated for destination filters
└── index.html              # UI layout updates
```

### Pattern 1: Filter Checks in Router Loop
**What:** Apply channel filter → source filter → destination filter per output before forwarding.
**When to use:** Every routed message in `routeMidiMessage()`.
**Example:**
```cpp
// Source: rp2040/midi_router.cpp (existing pattern)
if (source != MIDI_SOURCE_INTERNAL) {
    if (isMidiFiltered(static_cast<MidiInterfaceType>(source), msg.type)) {
        return;
    }
}

for (const auto &destEntry : interfaces) {
    if ((destMask & destEntry.mask) == 0) continue;
    // NEW: check destination filter matrix before forward
    if (isMidiDestFiltered(destEntry.iface, msg.type)) continue;
    forwardToInterface(destEntry.iface, msg);
}
```

### Pattern 2: Config JSON Round-Trip
**What:** Use `configToJson()` and `updateConfigFromJson()` to reflect runtime filter arrays.
**When to use:** Web Serial READALL/SAVEALL with validation.
**Example:**
```cpp
// Source: rp2040/config.cpp
JsonArray filters = doc["filters"].to<JsonArray>();
for (int iface = 0; iface < MIDI_INTERFACE_COUNT; ++iface) {
    JsonArray ifaceArr = filters.add<JsonArray>();
    for (int msg = 0; msg < MIDI_MSG_COUNT; ++msg) {
        ifaceArr.add(getMidiFilterState(iface, msg));
    }
}
```

### Anti-Patterns to Avoid
- **Reusing the source filter matrix for destination filtering:** breaks FILT-03 (source filters must remain unchanged) and makes UI semantics unclear.
- **Skipping JSON schema updates:** UI will silently reject new payloads or mis-parse arrays.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON parsing/validation | Custom string parsing | ArduinoJson + AJV (existing) | Avoids malformed config and brittle parsing |

**Key insight:** The config pipeline already has parsing, validation, and EEPROM storage paths; extend them instead of creating parallel flows.

## Common Pitfalls

### Pitfall 1: Inverted UI logic for filters
**What goes wrong:** UI checkboxes represent “allowed” but stored data is “blocked.”
**Why it happens:** `buildConfigJson()` stores `!checked` (see `web_configurator/src/app.ts`).
**How to avoid:** Maintain the same inversion when adding destination filters; document it in UI code.
**Warning signs:** Messages pass when the UI shows blocked or vice versa.

### Pitfall 2: EEPROM layout growth without size update
**What goes wrong:** New destination filter data overwrites IMU config bytes.
**Why it happens:** `CONFIG_EEPROM_SIZE` is fixed at 100 and assumes 24 bytes of filters.
**How to avoid:** Increase EEPROM size and keep save/load order aligned in `rp2040/config.cpp`.
**Warning signs:** IMU config loads garbage or debug logs show unexpected values.

### Pitfall 3: Destination filters not applied for internal sources
**What goes wrong:** Internal messages (IMU) bypass destination filters.
**Why it happens:** Destination filtering is done only in source filter block or skipped when source is internal.
**How to avoid:** Apply destination filters inside the per-destination loop, regardless of source.
**Warning signs:** IMU CCs ignore destination filter settings.

### Pitfall 4: Schema/UI mismatch for filter array size
**What goes wrong:** SAVEALL fails validation or firmware rejects config.
**Why it happens:** AJV schema expects 3x8 filters, firmware expects 6x8 (or vice versa).
**How to avoid:** Update both `validator.ts` and firmware JSON parsing together.
**Warning signs:** Web UI shows “Invalid configuration” on send.

## Code Examples

Verified patterns from the codebase:

### Channel + Source Filter Order
```cpp
// Source: rp2040/midi_router.cpp
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
```

### Filter Matrix Storage
```cpp
// Source: rp2040/midi_filters.cpp
static bool midiFilters[MIDI_INTERFACE_COUNT][MIDI_MSG_COUNT] = {0};
```

### UI Filter Serialization (Inverted Logic)
```ts
// Source: web_configurator/src/app.ts
arr.push(!(document.getElementById(`f-${iface}-${msg}`) as HTMLInputElement).checked);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Filter checks in each handler | Centralized `routeMidiMessage()` filter checks | Phase 1 | One location to add destination filtering |

**Deprecated/outdated:**
- Per-handler routing logic (already replaced by router).

## Open Questions

1. **Destination filter storage format in JSON/EEPROM**
   - What we know: Current JSON uses `filters` as 3x8 (source); EEPROM layout assumes 24 bytes.
   - What's unclear: Should destination filters be a new `destFilters` key or extend `filters` with a second matrix.
   - Recommendation: Add `destFilters` to avoid breaking existing clients; keep `filters` as source to satisfy FILT-03.

## Sources

### Primary (HIGH confidence)
- `rp2040/midi_filters.cpp` - filter storage, helper accessors
- `rp2040/midi_router.cpp` - routing order and current filter usage
- `rp2040/config.cpp` - JSON and EEPROM layout
- `web_configurator/src/app.ts` - UI serialization logic
- `web_configurator/src/validator.ts` - JSON schema assumptions

### Secondary (MEDIUM confidence)
- `.planning/REQUIREMENTS.md` - FILT-01/02/03 requirements
- `.planning/ROADMAP.md` - Phase 5 goal and success criteria

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - verified from repository usage
- Architecture: MEDIUM - inferred changes based on current router design
- Pitfalls: MEDIUM - derived from code inspection and config layout

**Research date:** 2026-02-25
**Valid until:** 2026-03-27
