# Phase 1 PoC — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the "Gallery-to-Gallery" loop with a working multi-video sequence and 3D LUT application.

**Architecture:** C++ Render Graph with a GLES 3.2 backend, executing Sequential and LUT nodes, orchestrated by an Android Compose shell with a modern Photo Picker and MediaStore export.

**Tech Stack:** C++20, OpenGL ES 3.2, Jetpack Compose, MediaCodec, MediaMuxer.

---

### Task 1: GLES 3.2 Backend & Render Graph Skeleton

**Files:**
- Create: `core/src/platform/gles/gles_backend.h`
- Create: `core/src/platform/gles/gles_backend.cpp`
- Create: `core/src/domain/render_graph.h`
- Create: `core/src/domain/render_graph.cpp`
- Modify: `core/CMakeLists.txt`

- [ ] **Step 1: Implement GlesBackend**
Provide GLES-specific implementation of `IGpuBackend`.
- [ ] **Step 2: Implement RenderGraph core**
Basic execution loop: `execute(time)` -> returns final `TextureHandle`.
- [ ] **Step 3: Register in CMake**
Wire `vx_platform_gles` static lib for Android.
- [ ] **Step 4: Commit**

---

### Task 2: Expanded Project Model (EDL)

**Files:**
- Modify: `core/src/domain/project.h`
- Create: `core/src/domain/sequence.h`
- Create: `core/src/domain/clip.h`
- Modify: `core/src/domain/project.cpp` (using nlohmann::json)

- [ ] **Step 1: Define MediaAsset, Sequence, and Clip**
Add these to the domain layer.
- [ ] **Step 2: Update JSON Serialization**
Replace the custom parser with `nlohmann::json` (ADR-0009) to handle nested arrays.
- [ ] **Step 3: Commit**

---

### Task 3: Android Photo Picker & JNI Wiring

**Files:**
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/MediaPool.kt`
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/engine/NativeBridge.kt`
- Modify: `android/app/src/main/cpp/jni_bindings.cpp`

- [ ] **Step 1: Implement Photo Picker in MediaPool**
Use `rememberLauncherForActivityResult(PickVisualMedia())`.
- [ ] **Step 2: Add `addMediaAsset(uri: String)` to NativeBridge**
- [ ] **Step 3: Implement JNI for asset addition**
Extract metadata (duration, resolution) via Android `MediaMetadataRetriever` and pass to C++ core.
- [ ] **Step 4: Commit**

---

### Task 4: Sequence & LUT Nodes

**Files:**
- Create: `core/src/domain/nodes/sequence_node.h`
- Create: `core/src/domain/nodes/lut_node.h`

- [ ] **Step 1: Implement SequenceNode**
Logic to select the correct decoder/texture based on timeline offset.
- [ ] **Step 2: Implement LutNode (GLES Fragment Shader)**
Write the shader for 3D LUT sampling (hardware trilinear).
- [ ] **Step 3: Commit**

---

### Task 5: Export Pipeline & MediaStore Integration

**Files:**
- Create: `android/app/src/main/kotlin/com/videoeditor/app/media/ExportService.kt`
- Modify: `android/app/src/main/kotlin/com/videoeditor/app/ui/EditorScreen.kt`

- [ ] **Step 1: Implement Export Loop**
Render frames -> Encoder -> Muxer -> MediaStore.
- [ ] **Step 2: Add Progress Modal**
Create the "Immersive Studio" style blocking dialog.
- [ ] **Step 3: Final End-to-End Verification**
Import 2 clips -> Apply LUT -> Export -> Check Gallery.
- [ ] **Step 4: Commit**
