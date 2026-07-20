# ADR-001: Proprietary engine without libobs

## Status
Accepted

## Context
RailShotTV must ship as a closed-source Windows product. OBS `libobs` is GPL.

## Decision
Build a custom engine on Qt 6 Widgets + Direct3D 11 + Media Foundation + WASAPI + LGPL FFmpeg.

## Consequences
More engineering cost; full control over licensing and product roadmap.
