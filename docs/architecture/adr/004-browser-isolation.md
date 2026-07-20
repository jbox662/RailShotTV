# ADR-004: Browser overlays isolation

## Status
Accepted

## Context
Browser sources (CEF/WebEngine) are crash-prone and GPU-contentious.

## Decision
Phase 4 browser overlays run in an isolated helper process communicating textures/frames
via shared GPU handles or shared memory. UI/engine must survive helper crashes.

## Consequences
Slightly higher latency for browser sources; much better stability.
Helper uses WebView2 when available (JS overlays), with QTextBrowser fallback.
