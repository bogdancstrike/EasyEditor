# Design Spec: Phase 1 — MVP Editor Working PoC

- **Status:** Approved
- **Date:** 2026-04-26
- **Topic:** Phase 1 End-to-End PoC (Render Graph Lite)

## 1. Goal
Implement a functional Proof of Concept (PoC) that covers the entire "Gallery-to-Gallery" workflow:
1.  **Import:** Select two videos from the Android Gallery.
2.  **Edit:** Sequence them back-to-end on a single-track timeline.
3.  **Enhance:** Apply a high-quality cinematic 3D LUT preset.
4.  **Export:** Render the sequence and save the resulting MP4 back to the Android Gallery.

## 2. Core Engine Architecture (Render Graph Lite)
The C++ core will transition from a single-clip decoder to a **Render Graph** driven engine.

### 2.1 GPU Backend (GLES 3.2)
- To unblock emulator development and support a wider range of Android devices (ADR-0004), we will implement `GlesBackend` as the primary functional backend for the PoC.
- It will implement the `IGpuBackend` interface, providing `TextureHandle` allocation and `dispatchCompute` (using fragment shaders or compute shaders if available in GLES 3.2).

### 2.2 Render Graph Nodes
- `SequenceNode`: Manages multiple `MediaCodec` decoder instances (via an abstract `ICodecBackend`). It switches decoders based on the timeline `t`.
- `LutNode`: Applies a 3D LUT using a fragment shader. The LUT data (33x33x33) will be hardcoded as a cinematic preset for the PoC.
- `TransformNode`: Handles basic aspect ratio fitting (Scale-to-fit/fill).

## 3. Android Platform Integration

### 3.1 Media Picker
- Use the modern **Android Photo Picker** (`PickVisualMedia`) for high-end UX and privacy.
- Supports multi-select of video files.
- URIs are passed to the C++ core for metadata extraction and sequence construction.

### 3.2 UI/UX Components
- **Progress Modal:** A blocking dialog during export that matches the "Immersive Studio" aesthetic.
    - Glassmorphism background blur.
    - Neon Pink linear progress bar.
    - Percentage text and "Exporting Cinematic Masterpiece..." status message.
- **Timeline Interaction:** Functional "snap" behavior when clips are positioned or trimmed (mocked in the initial UI task, made real in this PoC).

## 4. Export Pipeline
- Iterates through the timeline at the target FPS (e.g., 30fps).
- Executes the `RenderGraph` for each frame.
- Renders the output to a `MediaCodec` encoder input surface.
- Uses `MediaMuxer` to write the final MP4.
- Saves to `MediaStore.Video` to ensure immediate appearance in the user's Gallery.

## 5. Security & Privacy
- **Scoped Storage:** No `READ_EXTERNAL_STORAGE` permission required due to Photo Picker.
- **Privacy:** All processing happens entirely on-device (C++ engine).
