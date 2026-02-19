---
status: complete
phase: 01-generic-midi-router
source: 01-01-SUMMARY.md, 01-02-SUMMARY.md
started: 2026-02-19T03:43:41Z
updated: 2026-02-19T03:55:43Z
---

## Current Test

[testing complete]

## Tests

### 1. MIDI routing works across all interfaces
expected: Send a MIDI message from each source (USB Host device, USB Device from computer, Serial MIDI in). The message should be forwarded to the other two interfaces with the same filtering behavior as before the refactor, and activity LEDs should still respond.
result: pass

### 2. Note On vs Note Off routing remains correct
expected: Sending Note On and Note Off from any interface should route as before (note-offs still pass through correctly and don’t get treated as note-ons). There should be no stuck notes caused by routing changes.
result: issue
reported: "there is something wrong when serial midi notes gets passed to the usb device. It seems that some notes are stuck are some notes are not triggered at all. I see the logs in console about serial notes being registered in the rp2040"
severity: major

## Summary

total: 2
passed: 1
issues: 1
pending: 0
skipped: 0

## Gaps

- truth: "Note On and Note Off from any interface route correctly without stuck or missing notes"
  status: failed
  reason: "User reported: there is something wrong when serial midi notes gets passed to the usb device. It seems that some notes are stuck are some notes are not triggered at all. I see the logs in console about serial notes being registered in the rp2040"
  severity: major
  test: 2
  artifacts: []
  missing: []
