# RailShotTV Architecture

## Overview

RailShotTV is a proprietary Windows livestreaming engine. The React app under
`client/` is the UX reference prototype. The production app lives under `native/`.

```
UI (Qt Widgets)  →  EngineController  →  Capture / Compositor / Audio / Encode
                                         ↓
                              StreamSession + RecordingSession
                                         ↓
                                   HealthTelemetry → UI
```

## Threading model
- **UI thread**: Qt widgets, commands into `EngineController`
- **Render tick (~60 Hz)**: capture poll + D3D11 compose + submit to outputs
- **WASAPI capture threads**: mic + loopback → `AudioGraph`
- **RTMP writer thread**: bounded packet queue with reconnect/backoff
- **No capture/encode/network on the UI thread**

## Key modules
| Module | Path | Role |
|---|---|---|
| Domain | `native/src/core` | Project, SceneGraph, Settings, Secrets, Logger |
| Capture | `native/src/capture` | MF camera, Desktop Duplication, image/text |
| Compositor | `native/src/compositor` | D3D11 preview/program + transitions |
| Audio | `native/src/audio` | WASAPI, mix, meters, clock, monitor |
| Encoding | `native/src/encoding` | MF H.264, software fallback, AAC |
| Streaming | `native/src/streaming` | RTMP session + reconnect |
| Recording | `native/src/recording` | Crash-safe RSKV/MKV writer |
| Creator | `scoreboard/schedule/chat/overlays` | Phase 4 features |

## Persistence
- Projects: versioned JSON (`schemaVersion`)
- Settings: `QSettings`
- Secrets: Windows Credential Manager (`SecretStore`)
- Autosave every 30s to last project path

## ADRs
See `docs/architecture/adr/` for decisions on capture APIs, licensing, clocks.
