# TODO

> **How to use this file.** This is the rolling source of truth for what comes next. Keep tasks small enough that "done" is unambiguous. When a task is finished, move it from `Active` to `Completed` with the date. When a milestone is finished, archive its TODO section into `docs/archive/TODO-<milestone>.md` and start the next one fresh. **Do not let this file grow without bound.**
>
> **Linkage rules.** Every non-trivial task references either an ADR, a section of `docs/architecture.md`, or a GitHub issue. Tasks without context become stale within weeks.

---

## Status

- **Current milestone:** Phase 1 — MVP Editor
- **Target platforms:** Android only (iOS planned post-Phase 4; architecture stays portable)
- **Updated:** 2026-04-26

---

## Milestones (high-level)

Mirrors `docs/architecture.md` §21. Android-first; iOS work is deferred but **must not be foreclosed by Android-specific decisions**.

| # | Milestone | Status | Exit criteria |
|---|---|---|---|
| **0** | Foundations | 🟢 done | One clip plays at native fps on Android. Project save/reload byte-identical. CI green. |
| **1** | MVP Editor | 🟡 in progress | Import → trim → apply LUT → export Instagram Reel works end-to-end. |
| **2** | LUT & Preset System | ⚪ not started | Drop a preset pack folder → appears as one-tap preset. Hot-reload works. |
| **3** | Pro Features | ⚪ not started | Multi-track, transitions, custom export, full color tools, frame-rate conversion. |
| **4** | Polish, Audio, Release | ⚪ not started | Audio mix, HDR pipeline, accessibility, Play Store submission. |
| **5** | iOS port | ⚪ not started | Feature parity on iOS using the same C++ core. |

Legend: 🟢 done · 🟡 in progress · 🔴 blocked · ⚪ not started

---

## Phase 1 — MVP Editor (Working PoC)

### Active

- [ ] **Export Pipeline.** Replace the current MediaCodec transcode path with render-graph export so sequenced clips are exported with selected LUTs and saved to Android MediaStore (Gallery).
- [ ] **UI/UX Polish.** Add true drag-and-drop media placement from the media pool to the timeline, trim handles, snap behavior, and visual feedback.
- [ ] **Next UI task.** Add trim-handle gestures and snap feedback against the real project timeline.

### Completed

- [x] **2026-04-26 — README Refresh.** Updated `README.md` with project purpose, Phase 1 acceptance flow, current functionality, repository layout, architecture summary, APK build/install commands, and core test commands.
- [x] **2026-04-26 — Imported Cube LUTs.** Added DJI DLog to Rec709 plus Super 8 Daylight, Daylight Sharp, and Night `.cube` LUTs to Android raw resources and exposed them in the Look panel.
- [x] **2026-04-26 — Real 3D LUT Preview.** Switched Android live preview from hardcoded shader formulas to GLES 3D LUT texture sampling for built-in and imported cube looks.
- [x] **2026-04-26 — Media Pool vs Timeline Split.** Imported videos now stay in the media pool until explicitly added to the timeline; selected timeline clips can receive per-clip LUT overrides.
- [x] **2026-04-26 — Seekable Live Preview.** Added single-frame preview rendering for timeline scrubbing and playback start from the current cursor time.
- [x] **2026-04-26 — Real Timeline Data & Scrubbing.** Replaced mock timeline clips with `NativeBridge.clips`, scaled clip widths from actual durations, and added tap/drag seeking across the project duration.
- [x] **2026-04-26 — Floating Tool Dock & Expanded Preview.** Reworked the Android editor shell so the preview owns the full editor width, moved Media/Look/Effects/Adjust into a collapsible floating tool dock, and tuned the light/dark theme tokens for cleaner modern surfaces.
- [x] **2026-04-26 — MediaStore Export Skeleton.** Verified `ExportService` already implements a Phase 1 Android transcode path using `MediaExtractor`, `MediaCodec`, `MediaMuxer`, and `MediaStore`; LUT/render-graph export remains active.
- [x] **2026-04-26 — Sequence Playback Engine.** Fully wired the Compose UI to the C++ Render Graph. Implemented a 30fps playback loop in Kotlin that drives frame-accurate rendering via JNI and `ANativeWindow`. Added `androidx-compose-material-icons-extended` for playback controls.
- [x] **2026-04-26 — JNI Render Call & Surface Wiring.** Added `nativeRenderFrame` to `NativeBridge` and wired it through FFI to `ProjectService::renderFrame`. Integrated `ANativeWindow` handling in JNI and updated CMake to link against the Android native library.
- [x] **2026-04-26 — Sequence Playback Engine (Core Wiring).** Implemented `AndroidCodecBackend` using NDK `AMediaExtractor`. Wired `ICodecBackend` into `SequenceNode` for frame-accurate clip switching. Updated `RenderNode::render` and `RenderGraph::execute` to pass the codec backend reference.
- [x] **2026-04-26 — GLES 3.2 Fallback Backend.** Implemented `GlesBackend` with `RGBA16F` support and `updateTexture3D` optimization. Added RAII `GlTextureId` and `ScopedTexture` for safety.
- [x] **2026-04-26 — Expanded Project Model.** Implemented `MediaAsset`, `Sequence`, `Track`, and `Clip` with `nlohmann::json` serialization. Added full Doxygen documentation (ADR-0009).
- [x] **2026-04-26 — Android Media Picker.** Implemented modern Photo Picker in `MediaPool` with thread-safe (Dispatchers.IO) JNI wiring and metadata extraction.
- [x] **2026-04-26 — LUT Engine.** Implemented `LutNode` and a real GLES fragment shader for 3D LUT sampling. Optimized to reuse textures and avoid redundant allocations.
- [x] **2026-04-26 — Core Engine Skeleton.** Implemented `RenderGraph` core in the application layer, ensuring proper hexagonal layering (ADR-0006).

---

## Phase 2 — LUT & Preset System (planned)

- [ ] LUT registry with hot-reload.
- [ ] `FileObserver`-backed folder watcher (Android).
- [ ] LUT stacking with per-slot intensity + project-level LUT.
- [ ] Preset pack manifest (`.json`) parser with path-traversal hardening.
- [ ] Preset pack import via Storage Access Framework.
- [ ] Pro Mode toggle (free-placement timeline).

---

## Phase 3 — Pro Features (planned)

- [ ] Multi-track timeline (PiP, overlays, opacity, blend modes).
- [ ] Speed ramping per clip (constant first, then keyframed).
- [ ] Cross-dissolve, dip-to-black, wipe transitions.
- [ ] Custom export sheet (resolution, fps, bitrate, codec, color space).
- [ ] Frame-rate conversion (frame blending; optical flow behind feature flag).
- [ ] Whites, blacks, vibrance, temperature, tint.
- [ ] Sharpening (CAS port).

---

## Phase 4 — Polish, Audio, Release (planned)

- [ ] Audio tracks, fades, ducking, real-time meters, waveforms.
- [ ] Loudness normalization to target LUFS on export (libebur128).
- [ ] Text and title overlays.
- [ ] Background export with `WorkManager` and notification.
- [ ] HDR pipeline (Rec.2020 PQ/HLG end-to-end).
- [ ] Performance pass on low-end devices (Pixel 6a class).
- [ ] Accessibility pass (TalkBack, dynamic type).
- [ ] Play Store internal → closed → production rollout.

---

## Phase 5 — iOS Port (planned)

- [ ] `MetalBackend` implementing `IGpuBackend`.
- [ ] `AVFoundation`-backed `ICodecBackend`.
- [ ] Swift FFI wrapper around the C ABI in `core/include/vx/public_api.h`.
- [ ] SwiftUI mirror of the Compose UI (shared design system).
- [ ] CI: macOS runner, Xcode build + XCUITest.
- [ ] Pixel-comparison parity tests across both platforms (golden images shared).

---

## Cross-cutting (always-on)

- [ ] Add an ADR for every significant decision; never let one ship undocumented.
- [ ] Maintain the "weird files" test corpus under `test_assets/samples/edge_cases/`.
- [ ] Keep `docs/threading.md` in sync when threads or queues are added.
- [ ] Keep `docs/color.md` in sync when the color pipeline changes.
