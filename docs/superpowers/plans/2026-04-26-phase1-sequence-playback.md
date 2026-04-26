# Phase 1 PoC — Sequence Playback Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire `SequenceNode` and `RenderGraph` into the Compose UI for live preview of sequenced clips. Implement Android MediaCodec decoding via `ICodecBackend`.

**Architecture:** The Compose UI (`MediaCodecPreview`) will drive a playback loop. For each frame, it calls `NativeBridge.renderFrame(timeMs)`. The C++ core receives this, advances decoders via `ICodecBackend`, executes the `RenderGraph`, and presents the frame to the surface.

**Tech Stack:** C++20, JNI, MediaCodec (NDK `AMediaCodec`), Jetpack Compose.

---

### Task 1: C++ Codec Backend & Engine Wiring

**Files:**
- Modify: `core/src/platform/i_codec_backend.h`
- Create: `core/src/platform/android/android_codec_backend.h`
- Create: `core/src/platform/android/android_codec_backend.cpp`
- Modify: `core/CMakeLists.txt`

- [ ] **Step 1: Define `ICodecBackend` Interface**
Define methods for opening a file/URI and extracting a frame into a Gpu texture.
- [ ] **Step 2: Implement `AndroidCodecBackend`**
Use Android NDK `AMediaExtractor` and `AMediaCodec` to decode frames to an OpenGL texture (using `SurfaceTexture`).
- [ ] **Step 3: Update CMakeLists**
Add `vx_platform_android` library and compile `android_codec_backend.cpp` when on Android.
- [ ] **Step 4: Commit**

---

### Task 2: Sequence Node Decoding

**Files:**
- Modify: `core/src/application/nodes/sequence_node.cpp`
- Modify: `core/src/application/nodes/sequence_node.h`

- [ ] **Step 1: Wire Decoders to SequenceNode**
`SequenceNode` should use `ICodecBackend` to open clips and decode frames based on the current timeline `time`.
- [ ] **Step 2: Handle Transition**
When the `time` crosses from one clip to another, switch the active decoder.
- [ ] **Step 3: Commit**

---

### Task 3: JNI Render Call

**Files:**
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/engine/NativeBridge.kt`
- Modify: `android/app/src/main/cpp/jni_bindings.cpp`
- Modify: `core/include/vx/public_api.h`
- Modify: `core/src/ffi/project_ffi.cpp`
- Modify: `core/src/application/project_service.cpp`

- [ ] **Step 1: Add JNI and FFI for `renderFrame`**
Add `nativeRenderFrame(surface: Surface, timeMs: Long)` in `NativeBridge.kt`. Implement in `jni_bindings.cpp`, calling `vx_project_render_frame`.
- [ ] **Step 2: Implement Core Render Call**
`ProjectService` should execute the `RenderGraph` for the given time and present it to the output surface.
- [ ] **Step 3: Commit**

---

### Task 4: Compose UI Wiring

**Files:**
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/MediaCodecPreview.kt`
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/PreviewStage.kt`

- [ ] **Step 1: Update PreviewStage**
Drive playback via a coroutine loop that calls `NativeBridge.renderFrame` when playing.
- [ ] **Step 2: Update Surface Handling**
Ensure the `SurfaceTexture` from the `TextureView` is passed down to the native layer for rendering.
- [ ] **Step 3: Commit**
