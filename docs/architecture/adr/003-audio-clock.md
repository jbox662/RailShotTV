# ADR-003: Audio clock master

## Status
Accepted

## Context
A/V desync is the most common broadcast quality failure.

## Decision
`AudioClock` (steady_clock based, driven by WASAPI capture cadence) is the master timeline.
Video PTS are compared to audio clock; drift exposed in telemetry (`avDriftMs`).
Target: keep drift under 50 ms.

## Consequences
Encoder and recorder must timestamp against `AudioClock::nowUs()`.
