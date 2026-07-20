# ADR-002: Capture APIs

## Status
Accepted

## Context
Need reliable camera and screen capture on Windows 10/11.

## Decision
- Cameras: Media Foundation Source Reader
- Monitors: DXGI Desktop Duplication as primary reliable path
- Windows.Graphics.Capture as progressive enhancement for window capture
- Avoid GPU→CPU copies in the steady-state compose path

## Consequences
Desktop Duplication requires DXGI device affinity; handle ACCESS_LOST on mode changes.
