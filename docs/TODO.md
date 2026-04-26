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

- [ ] **Sequence Playback Engine.** Fully wire `SequenceNode` and `RenderGraph` into the Compose UI to enable live preview of sequenced clips beyond the initial skeleton.
- [ ] **Export Pipeline.** End-to-end export that renders the sequenced clips with LUTs and saves the result to the Android MediaStore (Gallery).
- [ ] **UI/UX Polish.** Functional timeline interactions: drag clips, snap, visual feedback for trimming.

### Completed

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
