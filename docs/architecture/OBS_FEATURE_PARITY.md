# OBS Feature Parity Matrix (RailShot)

Living SoT for OBS Studio frontend → RailShot Chromatic Command.
Status: `done` | `partial` | `todo` | `wont` (documented engine limit).

Last updated: Wave 0+1 ship.

## Design rules

- Behavior/layout from OBS (`obs-studio-master/frontend/`)
- Visuals: Chromatic Command (`docs/design/ORIGINAL_UI_RESTORE_SPEC.md`)
- Engine: proprietary Qt/D3D (no libobs)
- Game Capture = WGC/window (`wont` for DLL graphics-hook until proprietary path exists)

## Widgets (`frontend/widgets`)

| OBS | Role | Status | RailShot |
|-----|------|--------|----------|
| OBSBasic / OBSBasic_* | Main shell | partial | `MainWindow`, `DashboardPage`, `TopMenuBar` |
| OBSBasicControls | Stream/Record/Replay/VCam/Studio | partial | `BottomToolbar` + Go Live |
| OBSBasicStatusBar | FPS/CPU/dropped/bitrate/timers | partial | `ObsStatusBarWidget` |
| OBSBasicPreview | Interactive preview | partial | `PreviewWidget` |
| OBSBasic_StudioMode | Preview/Program | partial | Dual monitors; Studio Mode toggle |
| OBSBasic_Scenes | Scenes dock | partial | `SceneListWidget` + full toolbar |
| OBSBasic_SceneItems | Sources dock | partial | `InputTilesWidget` |
| AudioMixer | Mixer dock | partial | `AudioMixerWidget` |
| OBSBasic_Transitions | Transitions | partial | `TransitionPanel` |
| OBSBasicStats | Stats dock | partial | Analytics page |
| OBSProjector / Multiview | Projectors | todo | — |
| OBSBasic_ContextToolbar | Source context toolbar | partial | `SourceContextToolbar` |
| OBSBasic_Hotkeys | Hotkeys | partial | `HotkeyDispatcher` |
| OBSBasic_Recording/_Streaming/_ReplayBuffer/_VirtualCam | Outputs | partial | Engine + toolbar |
| OBSBasic_Screenshots | Screenshots | todo | — |
| OBSBasic_Projectors | Projector windows | todo | — |
| OBSBasic_Docks | Dock layout | partial | Nested QMainWindow docks |
| OBSBasic_Profiles / _SceneCollections | Profiles/collections | todo | Single project file |
| OBSBasic_Clipboard | Copy/paste transforms | partial | `TransformDialog` clipboard |
| OBSBasic_Dropfiles | Drag-drop media | todo | — |
| OBSBasic_Browser | Extra browsers | todo | — |
| ColorSelect | Color tags | partial | Properties swatches |

## Dialogs

| OBS | Status | RailShot |
|-----|--------|----------|
| OBSBasicSourceSelect | partial | `AddSourceDialog` |
| OBSBasicProperties | partial | `SourcePropertiesDialog` |
| OBSBasicFilters | partial | `FiltersDialog` |
| OBSBasicTransform | partial | `TransformDialog` |
| OBSBasicInteraction | todo | — |
| OBSBasicAdvAudio | todo | — |
| OBSBasicVCamConfig | todo | External/VCam toggle |
| OBSRemux / MissingFiles / LogViewer | todo | — |
| NameDialog | partial | QInputDialog |

## Wave progress

- [x] Wave 0 — this matrix
- [x] Wave 1 — Controls polish, StatusBar, Scenes toolbar, Context toolbar, Filters + Transform
- [ ] Wave 2 — Adv Audio + mixer depth
- [ ] Wave 3 — Preview power (lock, scale, projectors, interact)
- [ ] Wave 4 — Profiles / collections / remux / logs
- [ ] Wave 5 — Output/service polish
- [ ] Wave 6 — Extra browsers, dropfiles, hotkey UI, stats dock
