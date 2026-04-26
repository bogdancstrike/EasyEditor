# Mobile Video Editing App — Architecture & Implementation Plan

> **Document type:** Senior Software Architect technical specification
> **Scope:** Cross-platform (iOS + Android) professional-grade video editor with LUT-based color grading, project management, and customizable export pipeline.
> **Status:** Draft v1.0 — to be refined during Phase 0 architectural spike.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Important Clarifications and Assumptions](#2-important-clarifications-and-assumptions)
3. [Cross-Platform Strategy](#3-cross-platform-strategy)
4. [Technology Stack](#4-technology-stack)
5. [High-Level Architecture](#5-high-level-architecture)
6. [Core Data Model](#6-core-data-model)
7. [The Render Graph — Engine Core](#7-the-render-graph--engine-core)
8. [LUT and Preset System](#8-lut-and-preset-system)
9. [Decode, Playback, and Frame Cache](#9-decode-playback-and-frame-cache)
10. [Color Adjustments — Doing Them Right](#10-color-adjustments--doing-them-right)
11. [Timeline UI](#11-timeline-ui)
12. [Export Pipeline](#12-export-pipeline)
13. [Audio Subsystem](#13-audio-subsystem)
14. [Project Storage and Persistence](#14-project-storage-and-persistence)
15. [Performance Budget](#15-performance-budget)
16. [Security and Privacy](#16-security-and-privacy)
17. [Development Environment, Emulators, Tooling](#17-development-environment-emulators-tooling)
18. [Testing Strategy](#18-testing-strategy)
19. [CI/CD and Release Engineering](#19-cicd-and-release-engineering)
20. [Engineering Best Practices](#20-engineering-best-practices)
21. [Phased Roadmap](#21-phased-roadmap)
22. [Risks, Critiques, and Forward-Thinking Notes](#22-risks-critiques-and-forward-thinking-notes)

---

## 1. Executive Summary

This document describes the architecture and implementation plan for a professional mobile video editing application targeting **both iOS and Android with feature parity**. The application provides:

- Multi-track timeline editing (cut, split, trim, concat, drag-and-drop, snapping, magnetic and free-placement modes)
- Live GPU-accelerated preview with real-time effects
- Project management with non-destructive editing, undo/redo, and auto-save
- Color grading: exposure, contrast, highlights, shadows, whites, blacks, saturation, vibrance, temperature, tint, sharpness — performed in linear light for color correctness
- LUT-based color transforms (`.cube`, `.3dl`) with stacking and per-LUT intensity, including standard log-to-display transforms (e.g., D-Log → Rec.709)
- A folder-watching preset system that auto-imports user-supplied LUTs and preset packs
- Multi-format export with platform presets (Instagram, YouTube, TikTok) and full custom configuration (resolution, fps, bitrate, codec, color space)
- Audio mixing, fades, ducking, and waveform display

The architectural strategy is to **share the core engine (timeline, render graph, color science, codec orchestration) as a single C++ library** consumed by native UI shells on each platform. This guarantees feature parity, color-accurate consistency, and avoids paying the engineering cost twice.

**Estimated effort:** 3 months to credible MVP, 6–8 months to polished v1.0 with a small team (3–5 engineers).

---

## 2. Important Clarifications and Assumptions

### `.dng` is not a LUT format

The original requirements mention `.dng` for D-Log to Rec.709 conversion. `.dng` (Digital Negative) is Adobe's raw **image** format and is not used to ship LUTs. The industry-standard LUT formats are:

- **`.cube`** — Adobe / Resolve standard. The most common format. DJI, Sony, Canon, and Panasonic all distribute log-to-display LUTs as `.cube`.
- **`.3dl`** — Autodesk Lustre legacy format. Still common in post-production.
- **OCIO config bundles** — for advanced color-managed workflows.

**This document assumes `.cube` as the primary format with `.3dl` as a secondary import format.** If true raw `.dng` workflows are required (e.g., generating a LUT from a reference frame, or processing CinemaDNG sequences), this is a separate feature that can be added later.

### Magnetic vs. free-placement timeline

This is a deliberate UX choice that affects data model and gestures. The plan supports **both modes**, with magnetic as the default for casual users and a "Pro Mode" toggle that enables free placement. This decision must be made early because it affects the timeline data structure invariants.

### Scope boundary

What is described here is the feature set of CapCut + LumaFusion + a color-grading subset of DaVinci Resolve. This is a substantial product. The phased roadmap in [Section 21](#21-phased-roadmap) is structured to ship a credible MVP early and layer professional features on top.

---

## 3. Cross-Platform Strategy

### Decision: shared C++ core + native UI

```
┌─────────────────────────┐      ┌─────────────────────────┐
│   iOS App (Swift /      │      │ Android App (Kotlin /   │
│   SwiftUI)              │      │ Jetpack Compose)        │
└────────────┬────────────┘      └────────────┬────────────┘
             │ FFI (C ABI)                    │ JNI
             └────────────┬───────────────────┘
                          ▼
            ┌──────────────────────────────┐
            │  Shared Core (C++17/20)      │
            │  - Project model             │
            │  - Timeline / sequencer      │
            │  - Render graph              │
            │  - Color pipeline            │
            │  - LUT engine                │
            │  - Codec orchestration       │
            │  - Project serialization     │
            └──────────────────────────────┘
                          │
            ┌──────────────────────────────┐
            │ Platform abstractions        │
            │  GPU: Metal (iOS) /          │
            │       Vulkan (Android)       │
            │  Codec: AVFoundation /       │
            │         MediaCodec           │
            │  IO: NSFileManager /         │
            │      java.io / SAF           │
            └──────────────────────────────┘
```

### Why not Flutter / React Native?

These frameworks are inappropriate for this product class:

- **GPU access** to Metal/Vulkan is required for the render graph. Flutter's `Skia` and React Native's bridge add overhead and constrain what shaders you can run.
- **Hardware codec integration** (AVFoundation, MediaCodec) is needed at low level. Cross-platform wrappers exist but are leaky abstractions when you need frame-accurate seeking, format negotiation, and zero-copy texture import.
- **Performance**: a 60fps timeline + live preview at 4K cannot tolerate framework overhead.

### Why not Kotlin Multiplatform?

KMP is a credible alternative for the data layer (project model, serialization), but it does not give you GPU shaders or platform codec access. You would still need C++ for the engine, so KMP just adds a third language to maintain. **Recommendation: skip KMP, use C++ for the entire core.**

### Why C++ over Rust?

Rust would give safer pointer/lifetime handling around GPU resources. However:

- **FFmpeg, OpenColorIO, libplacebo, and most video libraries are C/C++** with mature bindings.
- **Apple and Google sample code** for Metal/Vulkan + codec integration is C++ first.
- **Hiring**: more mobile graphics engineers know C++ than Rust.
- **Mixing C++ and Rust** in one binary is possible but adds build complexity.

If the team has strong Rust experience, Rust is a defensible choice. For most teams, **C++17 (with selected C++20 features where well-supported) is the pragmatic recommendation.**

### Feature parity guarantee

To ensure both platforms ship the same features:

1. All editor logic lives in the C++ core. Platform code is "just UI."
2. The FFI layer is the **only** place where iOS and Android diverge — and it exposes identical functionality.
3. CI runs the same render-graph and pixel-comparison tests on both platforms.
4. Feature flags are defined in the core, not per-platform.

---

## 4. Technology Stack

### Core engine (shared)

| Concern | Technology | Rationale |
|---|---|---|
| Language | **C++17/20** | Industry standard for video, mature ecosystem |
| Build system | **CMake** + **Ninja** | Cross-platform, fast, integrates with Xcode and Android Studio |
| Package management | **Conan** or **vcpkg** | Reproducible third-party dependencies |
| Serialization | **nlohmann::json** for project files; **FlatBuffers** for caches | JSON is human-readable for project files; FlatBuffers for performance-critical caches |
| Color management | **OpenColorIO (OCIO) v2** | Industry-standard color management. Used by film studios. |
| LUT processing | **libplacebo** (optional) or in-house | High-quality LUT sampling and tone mapping |
| Logging | **spdlog** | Fast, header-only |
| Testing | **GoogleTest** + **Catch2** | GoogleTest for the engine, Catch2 for utilities |

### iOS

| Concern | Technology |
|---|---|
| Language | **Swift 5.9+** |
| UI | **SwiftUI** with **UIKit** interop where needed (timeline gestures benefit from `UIScrollView`) |
| GPU | **Metal 3** |
| Codec | **AVFoundation** (`AVAssetReader`, `AVAssetWriter`) |
| File access | **PHPickerViewController** (no permission required), `FileManager`, document-based app for LUT folder |
| Async | **Swift Concurrency** (`async/await`, `actor`) |
| Min target | **iOS 16.0** (covers ~95% of devices, gives Swift 5.9, modern Metal, Swift Concurrency) |

### Android

| Concern | Technology |
|---|---|
| Language | **Kotlin 1.9+** |
| UI | **Jetpack Compose** with `AndroidView` interop for `SurfaceView` |
| GPU | **Vulkan 1.1+** with **OpenGL ES 3.2** fallback for older devices |
| Codec | **MediaCodec** (async mode) + **MediaMuxer** |
| File access | **Photo Picker** API, **Storage Access Framework** for LUT folder |
| Async | **Kotlin Coroutines** + **Flow** |
| Min target | **Android API 28 (Android 9)** for Vulkan stability and modern MediaCodec; target API 34+ |
| NDK | **NDK r26+** for the C++ engine |

### Why Vulkan on Android (not OpenGL ES)?

- Lower CPU overhead, better multithreading
- Required for AV1 hardware decode integration via `AHardwareBuffer` on newer chips
- Better Vulkan-Metal portability (with MoltenVK as a development tool)
- OpenGL ES is deprecated by Khronos, declining vendor investment

Keep an OpenGL ES 3.2 fallback path for devices where Vulkan drivers are problematic (still common on low-end chipsets).

### Optional / advanced

- **FFmpeg (LGPL build)**: for import of formats not supported by hardware decoders (ProRes, DNxHD, some MKV variants). Adds ~5–10 MB to binary. **Defer to v2** unless users demand it.
- **TensorFlow Lite / Core ML**: for ML features (auto-captions, scene detection, object tracking, AI rotoscoping). Defer to a later phase.
- **Rubber Band Audio**: for high-quality time stretching with pitch preservation.

---

## 5. High-Level Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                         UI Layer (Native)                          │
│   iOS: SwiftUI + UIKit + Metal views                               │
│   Android: Jetpack Compose + SurfaceView                           │
│   - Timeline, Inspector, Media Pool, Effects panel, Export sheet   │
└────────────────────────────┬───────────────────────────────────────┘
                             │ FFI (C ABI, opaque handles)
┌────────────────────────────▼───────────────────────────────────────┐
│              Application Services Layer (C++)                      │
│   ProjectService    SequenceService    EffectsService              │
│   MediaImportService    ExportService    PresetService             │
└────────────────────────────┬───────────────────────────────────────┘
                             │
┌────────────────────────────▼───────────────────────────────────────┐
│                    Domain Core (C++)                               │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────────────┐ │
│  │ Project Model  │  │ Timeline /     │  │ Render Graph         │ │
│  │ (EDL)          │◄─┤ Sequencer      │◄─┤ (DAG of GPU passes)  │ │
│  └────────────────┘  └────────────────┘  └──────────┬───────────┘ │
│  ┌────────────────┐  ┌────────────────┐             │             │
│  │ LUT Registry   │  │ Color Pipeline │             │             │
│  └────────┬───────┘  └────────┬───────┘             │             │
│           └───────────────────┴──────────────────────┘             │
└────────────────────────────┬───────────────────────────────────────┘
                             │
┌────────────────────────────▼───────────────────────────────────────┐
│                Platform Abstraction Layer (C++)                    │
│   IGpuBackend     ICodecBackend     IFileSystemBackend             │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐     │
│   │ Metal impl   │  │AVFoundation  │  │ iOS FS impl          │     │
│   ├──────────────┤  ├──────────────┤  ├──────────────────────┤     │
│   │ Vulkan impl  │  │ MediaCodec   │  │ Android SAF impl     │     │
│   └──────────────┘  └──────────────┘  └──────────────────────┘     │
└────────────────────────────────────────────────────────────────────┘
```

### Architectural style

The core follows **Clean Architecture / Hexagonal Architecture** principles:

- **Domain core** (Project, Sequence, Clip, RenderGraph) has zero dependencies on platform code or frameworks.
- **Application services** orchestrate use cases (e.g., "import a video," "export the project") and coordinate domain objects.
- **Platform abstractions** are interfaces (`IGpuBackend`, `ICodecBackend`) implemented per platform.
- **UI** is a thin layer that calls into application services through the FFI.

This is the same pattern recommended for backend microservices, applied to a media application. The payoff is that you can unit-test the entire engine with a software-rasterizer GPU mock and a fake codec backend, with no device required.

### FFI (Foreign Function Interface)

Expose a C ABI from the core. Both Swift and Kotlin can bind to C cleanly.

```c
// public_api.h — what Swift/Kotlin sees
typedef struct VxProject VxProject;
typedef struct VxSequence VxSequence;

VxProject*  vx_project_create(const char* name);
void        vx_project_destroy(VxProject*);
const char* vx_project_serialize_json(VxProject*);
VxProject*  vx_project_load_json(const char* json);

VxSequence* vx_project_active_sequence(VxProject*);
int         vx_sequence_add_clip(VxSequence*, const char* asset_id,
                                 int64_t timeline_start_us,
                                 int64_t source_in_us, int64_t source_out_us);

void        vx_sequence_render_frame(VxSequence*, int64_t time_us,
                                     void* output_texture_handle);
```

On the Swift side, this is wrapped in idiomatic Swift classes. On the Android side, JNI wrappers expose the same surface to Kotlin.

---

## 6. Core Data Model

### Overview

The project is an **Edit Decision List (EDL)** — a description of what the output should be, expressed in terms of references to source media. Nothing in the model mutates source files. This is the fundamental property that enables non-destructive editing, undo/redo, and lossless re-export.

```cpp
// Conceptual model — full versions live in the core engine

struct Project {
    UUID id;
    std::string name;
    Timestamp created_at, modified_at;
    
    Resolution canvas;              // e.g. {1920, 1080}
    Rational framerate;             // e.g. 30000/1001 for 29.97
    ColorSpace color_space;         // Rec709, Rec2020_PQ, DisplayP3
    
    std::vector<MediaAsset> media_pool;
    std::vector<Sequence> sequences;
    UUID active_sequence_id;
    
    std::vector<LutInstance> project_lut_stack;
    ColorAdjustments project_color_adjustments;
    
    int schema_version;  // for migration
};

struct Sequence {
    UUID id;
    std::vector<Track> tracks;
    Time duration;  // computed
};

struct Track {
    UUID id;
    TrackType type;            // VIDEO or AUDIO
    std::vector<Clip> clips;   // sorted by start_time
    bool enabled;
    bool locked;
    float opacity;             // for video tracks
    BlendMode blend_mode;      // normal, multiply, screen, overlay...
};

struct Clip {
    UUID id;
    UUID asset_id;
    Time source_in;            // start in source media
    Time source_out;           // end in source media
    Time timeline_start;       // start on timeline
    float speed;               // 1.0 = normal, supports speed ramping later
    
    std::vector<LutInstance> lut_stack;
    ColorAdjustments color_adjustments;
    TransformParams transform;     // scale, position, rotation, crop, anchor
    std::vector<EffectInstance> effects;  // for additional effects
    
    AudioParams audio;
};
```

### Time as `Rational`

Use a rational number for time, not float. NTSC framerates are irrational in float (29.97 ≠ 30000/1001). Float drift causes audio-video sync bugs that are hard to track down.

```cpp
struct Time {
    int64_t ticks;
    int64_t timebase;  // common: 254016000000 (divisible by 24, 25, 30, 60, 120 etc.)
    
    static Time zero() { return {0, 254016000000LL}; }
    static Time fromSeconds(double s) { ... }
    static Time fromFrames(int64_t n, Rational fps) { ... }
    
    int64_t toFrames(Rational fps) const;
    double toSeconds() const;
};
```

### Schema versioning

Every project file has a `schema_version` field. Plan for migrations from day one:

```cpp
class ProjectMigrator {
public:
    Project migrate(const json& raw) {
        int v = raw.value("schema_version", 0);
        json data = raw;
        while (v < CURRENT_SCHEMA_VERSION) {
            data = migrations_[v](data);
            v++;
        }
        return Project::fromJson(data);
    }
};
```

A v1 project must always open in a v5 app. Test this in CI.

---

## 7. The Render Graph — Engine Core

### The principle

For any timeline time `t`, the engine produces a **render graph**: a DAG of GPU operations whose output is the frame for that time. Both live preview and final export consume the same graph; they differ only in output resolution and codec.

```
For time t = 4.5s:

  [Decode Clip A @ source_t=2.1s] ──► [Transform] ──► [Color adjust]
                                                            │
                                                            ▼
                                                     [LUT: D-Log→Rec.709]
                                                            │
                                                            ▼
                                                     [LUT: Iceland Look @ 0.7]
                                                            │
                                                            ▼
                                                  [Composite onto Track 1]
                                                            │
  [Decode Clip B @ source_t=0.3s] ──► [Transform PiP] ──► ──┤ (overlay)
                                                            │
                                                            ▼
                                                  [Project LUT stack]
                                                            │
                                                            ▼
                                                     [Final frame]
```

### Why a graph, not a hardcoded pipeline

- **Dynamic effect stacks**: users add and remove effects at runtime; the pipeline must reconfigure.
- **Caching**: if only the project-level LUT changed, per-clip work is cached and not re-run.
- **Optimization**: adjacent shader passes can be fused. Linear color operations on the same color space can collapse into a single shader.
- **Quality scaling**: the same graph runs at 540p during scrubbing and full resolution during export.

### Implementation sketch

```cpp
class RenderNode {
public:
    virtual ~RenderNode() = default;
    virtual TextureHandle render(RenderContext& ctx,
                                 std::span<const TextureHandle> inputs) = 0;
    virtual uint64_t cacheKey() const = 0;
    virtual NodeKind kind() const = 0;
};

class LutNode : public RenderNode {
    LutData lut_;
    float intensity_;
public:
    TextureHandle render(RenderContext& ctx,
                         std::span<const TextureHandle> inputs) override {
        auto out = ctx.allocateTexture(inputs[0].size(), PixelFormat::RGBA16F);
        ctx.dispatchCompute("lut_apply", {inputs[0], lut_.gpuTexture}, out,
                            ShaderConstants{{"intensity", intensity_}});
        return out;
    }
    uint64_t cacheKey() const override {
        return hash_combine(lut_.id, intensity_);
    }
    NodeKind kind() const override { return NodeKind::LUT; }
};

class RenderGraph {
    std::vector<std::unique_ptr<RenderNode>> nodes_;
    std::vector<std::vector<size_t>> edges_;  // adjacency: edges_[i] = parents of i
    LRUCache<uint64_t, TextureHandle> cache_;
    
public:
    TextureHandle execute(RenderContext& ctx, size_t output_idx) {
        return executeNode(ctx, output_idx);
    }
    
private:
    TextureHandle executeNode(RenderContext& ctx, size_t idx) {
        auto& node = nodes_[idx];
        auto key = node->cacheKey();
        if (auto cached = cache_.get(key)) return *cached;
        
        std::vector<TextureHandle> inputs;
        for (auto parent_idx : edges_[idx]) {
            inputs.push_back(executeNode(ctx, parent_idx));
        }
        
        auto result = node->render(ctx, inputs);
        cache_.put(key, result);
        return result;
    }
};
```

### Working in linear/scene-referred space

Intermediate textures are **RGBA16F** (half-float). This preserves headroom for over-bright values, prevents clipping when stacking effects, and supports HDR pipelines. Final output is converted to the destination color space and bit depth before encoding.

### GPU backend abstraction

```cpp
class IGpuBackend {
public:
    virtual TextureHandle allocateTexture(Size, PixelFormat) = 0;
    virtual void dispatchCompute(std::string_view shader_name,
                                 std::span<const TextureHandle> inputs,
                                 TextureHandle output,
                                 const ShaderConstants&) = 0;
    virtual void copyTexture(TextureHandle src, TextureHandle dst) = 0;
    virtual void waitForGpu() = 0;
    virtual ~IGpuBackend() = default;
};

class MetalBackend : public IGpuBackend { ... };
class VulkanBackend : public IGpuBackend { ... };
```

Shaders are written **once in a portable form** and translated:

- Author shaders in **GLSL** or **WGSL**.
- Use **SPIRV-Cross** to convert SPIR-V → MSL (Metal Shading Language) at build time.
- For Vulkan, compile GLSL → SPIR-V with `glslangValidator`.

This keeps shader source DRY and color-correct on both platforms.

---

## 8. LUT and Preset System

### Folder watching

Users drop `.cube` files into a designated folder; the app imports them automatically.

**iOS implementation:**
- Configure the app as a document-based application via `UIFileSharingEnabled` and `LSSupportsOpeningDocumentsInPlace` in `Info.plist`. Folder appears under `Files → On My iPhone → [App] → LUTs`.
- Watch with `DispatchSource.makeFileSystemObjectSource` on the directory, or `NSMetadataQuery` for iCloud-backed folders.

**Android implementation:**
- Use the **Storage Access Framework** to let users pick a folder once and grant persistent URI permission.
- Watch with `FileObserver` (legacy paths) or `ContentObserver` on `MediaStore`. For app-specific external storage, simpler: use `FileObserver` directly.

```kotlin
class LutFolderWatcher(
    private val folderPath: String,
    private val registry: LutRegistry
) {
    private val observer = object : FileObserver(folderPath,
        CREATE or DELETE or MOVED_TO or MOVED_FROM) {
        override fun onEvent(event: Int, path: String?) {
            if (path == null) return
            val lower = path.lowercase()
            if (lower.endsWith(".cube") || lower.endsWith(".3dl")) {
                registry.rescan()
            }
        }
    }
    fun start() = observer.startWatching()
    fun stop() = observer.stopWatching()
}
```

### `.cube` parsing

```cpp
struct Lut3D {
    int size;
    std::array<float, 3> domain_min;
    std::array<float, 3> domain_max;
    std::vector<float> data;  // size^3 * 3 floats
    std::string title;
    std::filesystem::path source;
};

Lut3D parseCube(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open " + path.string());
    
    Lut3D lut;
    lut.size = -1;
    lut.domain_min = {0.f, 0.f, 0.f};
    lut.domain_max = {1.f, 1.f, 1.f};
    lut.title = path.stem().string();
    
    std::string line;
    while (std::getline(in, line)) {
        // trim, skip blanks and #-comments
        // parse TITLE, LUT_3D_SIZE, DOMAIN_MIN, DOMAIN_MAX
        // collect float triples into lut.data
    }
    
    if (lut.size <= 0)
        throw std::runtime_error("LUT_3D_SIZE missing in " + path.string());
    if ((int)lut.data.size() != lut.size * lut.size * lut.size * 3)
        throw std::runtime_error("LUT data size mismatch in " + path.string());
    
    return lut;
}
```

Once parsed, the LUT is uploaded as a **3D texture** to the GPU. Hardware trilinear sampling does the interpolation.

### LUT shader (Metal example)

```metal
#include <metal_stdlib>
using namespace metal;

struct LutParams { float intensity; };

kernel void apply_lut_3d(
    texture2d<float, access::read>   src    [[texture(0)]],
    texture3d<float, access::sample> lut    [[texture(1)]],
    texture2d<float, access::write>  dst    [[texture(2)]],
    constant LutParams&              params [[buffer(0)]],
    uint2                            gid    [[thread_position_in_grid]]
) {
    if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) return;
    
    float4 input = src.read(gid);
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    float3 graded = lut.sample(s, clamp(input.rgb, 0.0, 1.0)).rgb;
    float3 mixed = mix(input.rgb, graded, params.intensity);
    dst.write(float4(mixed, input.a), gid);
}
```

### Preset packs (stacking)

A "preset pack" is a JSON manifest plus referenced LUT files, optionally bundled in a `.zip` or shipped as a folder:

```json
{
  "schema_version": 1,
  "name": "Iceland Cinematic",
  "author": "Bogdan",
  "description": "Cool teal-orange grade for outdoor footage shot in DJI D-Log",
  "stack": [
    { "type": "lut", "file": "DJI_DLog_to_Rec709.cube", "intensity": 1.0 },
    { "type": "color_adjust", "exposure": 0.2, "contrast": 15, "shadows": -10 },
    { "type": "lut", "file": "Iceland_Cyan_Orange.cube", "intensity": 0.7 }
  ]
}
```

The user drops the folder into the LUT directory; the watcher detects the manifest, validates it, and registers it as a single one-tap preset that internally expands to the full effect chain.

### Color-correctness warning

Stacking two display-referred LUTs (clamped to 0–1) can lose highlight detail. The expected pattern is:

1. **Transform LUT first** (log → display, e.g., D-Log → Rec.709) to convert from a log encoding to a display-referred representation.
2. **Look LUT second** (creative grade) on top.

If a user stacks two creative LUTs, surface a soft warning in the UI. For v2, consider a true scene-referred working space (ACES or linear sRGB-extended) where intermediate values are not clamped.

---

## 9. Decode, Playback, and Frame Cache

### Phone codec realities

Mobile hardware decoders are excellent at forward playback of the codec they were designed for. They are weaker at:

- Random seeks to non-keyframes in long-GOP files
- Decoding multiple streams simultaneously (typical mobile SoC: 1–4 hardware decoder instances; older devices: 1)
- 10-bit, HDR, ProRes, DNxHD, and other "post-production" codecs

This shapes the strategy.

### Strategy: proxies + frame cache + hardware decode

**On import**, generate a low-resolution proxy in the background:

```
User imports 4K H.265 clip
  ├─ Register MediaAsset → immediately playable from original
  ├─ Background: decode → downscale to 540p → re-encode H.264 baseline
  ├─ Save to <cache>/proxies/<asset_id>.mp4
  ├─ When proxy ready → editor switches transparently
  └─ Export pipeline always uses original
```

**Frame cache:** keep the last N decoded frames in GPU textures (N ≈ 60 frames ≈ 2s at 30fps). Scrubbing serves from cache; jumps trigger decoder seeks.

### iOS decode (zero-copy to Metal)

```swift
final class FrameExtractor {
    private let asset: AVAsset
    private var reader: AVAssetReader?
    private var output: AVAssetReaderTrackOutput?
    private let textureCache: CVMetalTextureCache
    
    func decodeFrame(at time: CMTime) async throws -> MTLTexture {
        if reader == nil || time < currentReaderHead {
            try recreateReader(seekingTo: time)
        }
        
        while let buffer = output?.copyNextSampleBuffer() {
            let pts = CMSampleBufferGetPresentationTimeStamp(buffer)
            if CMTimeCompare(pts, time) >= 0 {
                guard let pb = CMSampleBufferGetImageBuffer(buffer) else { continue }
                return try makeMetalTexture(from: pb)
            }
        }
        throw FrameExtractorError.endOfStream
    }
    
    private func makeMetalTexture(from pb: CVPixelBuffer) throws -> MTLTexture {
        var cvTex: CVMetalTexture?
        let w = CVPixelBufferGetWidth(pb)
        let h = CVPixelBufferGetHeight(pb)
        let status = CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault, textureCache, pb, nil,
            .bgra8Unorm, w, h, 0, &cvTex)
        guard status == kCVReturnSuccess, let cvTex,
              let mtl = CVMetalTextureGetTexture(cvTex)
        else { throw FrameExtractorError.textureCreationFailed }
        return mtl
    }
}
```

### Android decode (zero-copy via SurfaceTexture / AHardwareBuffer)

```kotlin
class FrameExtractor(private val source: Uri) {
    private val codec = MediaCodec.createDecoderByType(mimeType)
    private val outputSurface = createSurface()  // backed by SurfaceTexture/AHardwareBuffer
    
    suspend fun decodeFrame(timeUs: Long): TextureHandle = withContext(Dispatchers.Default) {
        // Seek extractor to nearest keyframe ≤ timeUs
        // Feed input buffers; collect output until pts >= timeUs
        // Output is rendered to outputSurface; bind as GL_TEXTURE_EXTERNAL_OES
        // or import to Vulkan via VK_KHR_external_memory_android_hardware_buffer
    }
}
```

### Playback loop

Single producer/consumer on two threads:

```
[Decode thread]                          [Render thread @ display vsync]
    │                                            │
    ▼                                            ▼
Seek/decode source ──► Ring buffer ◄──── Pick frame for current time t
                       (pts, texture)            │
                                                 ▼
                                          Walk RenderGraph(t)
                                                 │
                                                 ▼
                                          Present to screen
```

If the decoder falls behind, drop preview frames (skip rendering some) but keep the audio playing — **audio is the master clock**. Resync video to audio, never the reverse.

---

## 10. Color Adjustments — Doing Them Right

Most casual editors apply exposure/contrast/shadows in non-linear sRGB space. The result looks muddy and hue-shifts in highlights. Professional editors operate in **linear light**.

### The pipeline

```glsl
vec3 color = texture(input, uv).rgb;

// 1. Decode from source EOTF to linear
color = srgb_to_linear(color);   // or rec709_to_linear, etc.

// 2. Exposure (multiplicative — physically correct in linear)
color *= pow(2.0, params.exposure);   // exposure in stops

// 3. White balance (shift via matrix in CIE XYZ or simple RGB scaling)
color = applyWhiteBalance(color, params.temperature, params.tint);

// 4. Contrast around middle gray (0.18 in linear scene-referred light)
const float pivot = 0.18;
color = (color - pivot) * (1.0 + params.contrast) + pivot;

// 5. Highlights / shadows / whites / blacks via tone curve
color = applyToneCurve(color, params);

// 6. Saturation around luminance (Rec.709 weights for HD content)
float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
color = mix(vec3(luma), color, 1.0 + params.saturation);

// 7. Vibrance (saturation that protects skin tones, weaker on already-saturated colors)
color = applyVibrance(color, params.vibrance);

// 8. Re-encode to display
color = linear_to_srgb(color);   // or apply final color space transform
```

### Highlights / shadows / whites / blacks

Naive multiply-by-region produces crunchy results. Better: a smooth piecewise tone curve.

A tested approach is the **luminance-masked region adjustment**:

```glsl
float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));

// Smooth weight functions
float wHighlights = smoothstep(0.5, 1.0, luminance);
float wShadows    = 1.0 - smoothstep(0.0, 0.5, luminance);
float wWhites     = smoothstep(0.7, 1.0, luminance);
float wBlacks     = 1.0 - smoothstep(0.0, 0.3, luminance);

color += params.highlights * wHighlights * 0.1;
color += params.shadows    * wShadows    * 0.1;
color += params.whites     * wWhites     * 0.1;
color += params.blacks     * wBlacks     * 0.1;
```

Tune the constants empirically. For a more refined approach, study Darktable's `tonecurve` and `colorbalancergb` modules — they are open source and represent best practice.

### Sharpening (unsharp mask)

```glsl
vec3 sharpen(sampler2D input, vec2 uv, float amount, float radius) {
    vec3 center = texture(input, uv).rgb;
    vec3 blurred = gaussianBlur(input, uv, radius);
    return center + (center - blurred) * amount;
}
```

For better-quality sharpening, consider an edge-aware sharpener (e.g., CAS — Contrast Adaptive Sharpening from AMD, open source).

---

## 11. Timeline UI

### Layered rendering

Three layers, drawn separately for performance:

1. **Ruler / playhead** — static-ish, redrawn on zoom change.
2. **Clip strips** — the bulk of the timeline. Each clip is a rounded rectangle containing a thumbnail strip pre-rendered at import time.
3. **Interaction layer** — selection, drag handles, gestures.

### Thumbnail strips

At import, render N thumbnails (e.g., 100 at 90×60) into a single tall PNG. At display time, sample columns based on the clip's `source_in/source_out`.

```
Clip's visible thumbnail = sample column floor(((screenX - clipX) / clipWidth) * N)
```

This keeps timeline scrolling at 120fps even on cheap devices.

### Snapping

When dragging a clip, snap edges to:

- Other clip edges (same and adjacent tracks)
- The playhead
- Sequence markers
- Frame boundaries (always)
- Major time grid marks (every second, 5s, 10s — depends on zoom)

Snapping engages within ~8 device pixels. This is what makes timelines feel professional.

### Magnetic vs. free placement

```
Magnetic mode (default):
  - No gaps between clips on a track
  - No overlaps on the same track
  - Inserting shifts later clips
  - Friendly for casual users (CapCut, iMovie style)

Free placement (Pro Mode):
  - Arbitrary positions
  - Overlaps allowed (with explicit transitions)
  - Required for advanced editing (Premiere, Final Cut style)
```

The data model supports both; the difference is invariants enforced when committing edits.

### iOS sketch

```swift
struct TimelineView: View {
    @ObservedObject var sequence: SequenceVM
    @State private var pixelsPerSecond: Double = 100.0
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            ZStack(alignment: .topLeading) {
                RulerView(pps: pixelsPerSecond)
                    .frame(height: 28)
                
                VStack(alignment: .leading, spacing: 4) {
                    ForEach(sequence.tracks) { track in
                        TrackLaneView(track: track, pps: pixelsPerSecond)
                    }
                }
                .padding(.top, 28)
                
                PlayheadView(time: sequence.currentTime, pps: pixelsPerSecond)
            }
        }
        .gesture(
            MagnificationGesture()
                .onChanged { v in
                    pixelsPerSecond = max(20, min(2000, basePps * v))
                }
        )
    }
}
```

### Android sketch

```kotlin
@Composable
fun TimelineView(sequence: SequenceVM) {
    var pps by remember { mutableStateOf(100f) }
    val basePps = remember { 100f }
    
    Box(modifier = Modifier
        .fillMaxSize()
        .pointerInput(Unit) {
            detectTransformGestures { _, _, zoom, _ ->
                pps = (pps * zoom).coerceIn(20f, 2000f)
            }
        }) {
        Row(modifier = Modifier.horizontalScroll(rememberScrollState())) {
            Column {
                RulerView(pps)
                sequence.tracks.forEach { TrackLaneView(it, pps) }
            }
        }
        Playhead(time = sequence.currentTime, pps = pps)
    }
}
```

---

## 12. Export Pipeline

### Overview

Export iterates over timeline time, renders each frame through the same render graph used for preview, encodes via hardware, and muxes with audio.

```
[Timeline iterator: t = 0, 1/fps, 2/fps, ...]
        │
        ▼
[Render graph at full output resolution]
        │
        ▼
[Texture → encoder input]
        │
        ▼                                ┌────────────────────┐
[Hardware video encoder]                 │ Audio mixer chain  │
   H.264 / HEVC / AV1                    │ + AAC encoder      │
        │                                └─────────┬──────────┘
        └─────────────┬────────────────────────────┘
                      ▼
              [Muxer → MP4/MOV file]
                      │
                      ▼
              [Save / share]
```

### iOS export

```swift
func export(sequence: Sequence, settings: ExportSettings, to url: URL) async throws {
    let writer = try AVAssetWriter(outputURL: url, fileType: settings.containerType)
    
    let videoInput = AVAssetWriterInput(mediaType: .video, outputSettings: [
        AVVideoCodecKey: settings.codec.avCodec,        // .h264, .hevc
        AVVideoWidthKey: settings.resolution.width,
        AVVideoHeightKey: settings.resolution.height,
        AVVideoCompressionPropertiesKey: [
            AVVideoAverageBitRateKey: settings.bitrate,
            AVVideoMaxKeyFrameIntervalKey: settings.fps * 2,
            AVVideoProfileLevelKey: settings.profileLevel,
            AVVideoColorPropertiesKey: settings.colorProperties,
        ]
    ])
    videoInput.expectsMediaDataInRealTime = false
    
    let pixelAdaptor = AVAssetWriterInputPixelBufferAdaptor(
        assetWriterInput: videoInput,
        sourcePixelBufferAttributes: [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA,
            kCVPixelBufferWidthKey as String: settings.resolution.width,
            kCVPixelBufferHeightKey as String: settings.resolution.height,
            kCVPixelBufferMetalCompatibilityKey as String: true,
        ])
    
    let audioInput = AVAssetWriterInput(mediaType: .audio, outputSettings: [
        AVFormatIDKey: kAudioFormatMPEG4AAC,
        AVSampleRateKey: 48000,
        AVNumberOfChannelsKey: 2,
        AVEncoderBitRateKey: settings.audioBitrate,
    ])
    
    writer.add(videoInput); writer.add(audioInput)
    writer.startWriting()
    writer.startSession(atSourceTime: .zero)
    
    let frameInterval = CMTime(value: 1, timescale: Int32(settings.fps))
    var current = CMTime.zero
    let endTime = sequence.duration.cmTime
    
    while current < endTime {
        while !videoInput.isReadyForMoreMediaData {
            try await Task.sleep(nanoseconds: 1_000_000)
        }
        let texture = renderEngine.renderFrame(sequence: sequence, at: current,
                                               resolution: settings.resolution)
        let pb = pixelBufferPool.makeBuffer(from: texture)
        pixelAdaptor.append(pb, withPresentationTime: current)
        current = current + frameInterval
        
        await MainActor.run { progress.value = current.seconds / endTime.seconds }
    }
    
    // Audio path runs in parallel, similar pattern
    
    videoInput.markAsFinished()
    audioInput.markAsFinished()
    await writer.finishWriting()
}
```

### Android export

```kotlin
suspend fun export(sequence: Sequence, settings: ExportSettings, output: File) {
    val format = MediaFormat.createVideoFormat(settings.mimeType,
        settings.width, settings.height).apply {
        setInteger(MediaFormat.KEY_BIT_RATE, settings.bitrate)
        setInteger(MediaFormat.KEY_FRAME_RATE, settings.fps)
        setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 2)
        setInteger(MediaFormat.KEY_COLOR_FORMAT,
                   MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
    }
    
    val encoder = MediaCodec.createEncoderByType(settings.mimeType)
    encoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
    val inputSurface = encoder.createInputSurface()
    encoder.start()
    
    val muxer = MediaMuxer(output.absolutePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
    
    // Render frames to inputSurface from your Vulkan/GL context.
    // Drain encoder buffers; write to muxer.
    // Audio path in parallel via separate AAC encoder.
}
```

### Export presets

JSON-driven, easy to extend without code changes:

```json
{
  "version": 1,
  "presets": [
    {
      "id": "instagram_reel",
      "name": "Instagram Reel / TikTok",
      "container": "mp4",
      "video": {
        "codec": "h264", "profile": "high",
        "resolution": [1080, 1920], "fps": 30,
        "bitrate": 6000000, "color_space": "rec709"
      },
      "audio": { "codec": "aac", "sample_rate": 48000, "bitrate": 192000 }
    },
    {
      "id": "instagram_feed_square",
      "name": "Instagram Feed (Square)",
      "container": "mp4",
      "video": {
        "codec": "h264", "resolution": [1080, 1080],
        "fps": 30, "bitrate": 5000000
      },
      "audio": { "codec": "aac", "sample_rate": 48000, "bitrate": 192000 }
    },
    {
      "id": "youtube_1080p",
      "name": "YouTube 1080p",
      "container": "mp4",
      "video": {
        "codec": "h264", "profile": "high",
        "resolution": [1920, 1080], "fps": 30,
        "bitrate": 12000000
      },
      "audio": { "codec": "aac", "sample_rate": 48000, "bitrate": 384000 }
    },
    {
      "id": "youtube_4k_hdr",
      "name": "YouTube 4K HDR",
      "container": "mp4",
      "video": {
        "codec": "hevc", "profile": "main10",
        "resolution": [3840, 2160], "fps": 60,
        "bitrate": 50000000, "color_space": "rec2020_pq"
      },
      "audio": { "codec": "aac", "sample_rate": 48000, "bitrate": 384000 }
    },
    {
      "id": "custom",
      "name": "Custom",
      "user_configurable": true
    }
  ]
}
```

The Custom preset allows: any resolution, fps from {24, 25, 30, 50, 60, 120}, bitrate in CBR or VBR with a quality target, choice of codec (H.264, HEVC, AV1 if device supports), and color space.

### Frame-rate conversion

When timeline fps differs from source fps, do not just drop or duplicate frames.

- **Frame blending** (default): linear interpolation between two source frames around the target time.
- **Optical flow** (high-quality option): synthesize intermediate frames from motion vectors. iOS has `VNGenerateOpticalFlowRequest`. On Android, there is no built-in equivalent — use TensorFlow Lite with a model like FILM or RIFE, or a custom GPU implementation.

---

## 13. Audio Subsystem

A video editor without proper audio is not credible. Minimum features:

- Per-clip volume + fade in/out
- Master volume per audio track
- Background music track separate from clip audio
- Real-time audio meters during playback (peak + LUFS)
- Audio waveforms on the timeline (pre-rendered at import)
- Simple ducking: reduce music volume when speech track is loud

### Implementation

- **iOS**: `AVAudioEngine` for the mixer graph, `AVAudioPlayerNode` per source.
- **Android**: **Oboe** library (Google-recommended; wraps AAudio with OpenSL ES fallback). Lower latency than higher-level Android audio APIs.

For advanced features:

- **Pitch-preserving time stretch** (clip speed changes that keep voice natural): SoundTouch (LGPL) or Rubber Band (commercial / GPL).
- **Loudness normalization** (target -14 LUFS for Spotify/social, -23 LUFS for broadcast): use libebur128.

### Why audio matters more than people think

Users will ship videos with terrible audio levels and blame the app. A simple "Auto-normalize to -14 LUFS" toggle on export goes a very long way.

---

## 14. Project Storage and Persistence

### Filesystem layout

```
<app_documents>/Projects/<project_uuid>/
  project.json              # the EDL — versioned, human-readable
  thumbnail.jpg             # for project picker
  cache/
    proxies/<asset_id>.mp4  # 540p proxies for editing
    thumbnails/<asset_id>_strip.jpg  # tall thumbnail strips
    waveforms/<asset_id>.dat
    rendergraph/<hash>.bin  # cached intermediate frames (rare; usually in-memory only)
  autosave/
    project_<timestamp>.json  # rolling backups, last 5 kept
```

### Project file format

JSON with explicit schema versioning:

```json
{
  "schema_version": 1,
  "project": {
    "id": "uuid",
    "name": "My Reel",
    "created_at": "2026-04-26T10:00:00Z",
    "modified_at": "2026-04-26T10:30:00Z",
    "canvas": [1920, 1080],
    "framerate": { "num": 30000, "den": 1001 },
    "color_space": "rec709",
    "media_pool": [...],
    "sequences": [...],
    "active_sequence_id": "uuid"
  }
}
```

### Auto-save

- Every 30 seconds during active editing
- After every meaningful action (clip added, trim committed, effect added)
- Keep the last 5 auto-saves; never delete the manual save
- On launch, detect crash recovery: if a session ended without a clean save, offer to restore the most recent auto-save

### Undo / redo (Command pattern)

Non-negotiable for a serious editor.

```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void apply(Project&) = 0;
    virtual void revert(Project&) = 0;
    virtual std::string description() const = 0;  // for "Undo Trim" menu label
};

class CommandStack {
    std::vector<std::unique_ptr<Command>> undo_stack_;
    std::vector<std::unique_ptr<Command>> redo_stack_;
    static constexpr size_t MAX_DEPTH = 200;
public:
    void execute(std::unique_ptr<Command> cmd, Project& project) {
        cmd->apply(project);
        undo_stack_.push_back(std::move(cmd));
        redo_stack_.clear();
        if (undo_stack_.size() > MAX_DEPTH)
            undo_stack_.erase(undo_stack_.begin());
    }
    void undo(Project& project) { ... }
    void redo(Project& project) { ... }
};
```

Every state mutation in the application goes through the command stack. There must be no "set this field directly" code path that bypasses commands, or undo will be inconsistent.

---

## 15. Performance Budget

A 60fps timeline + live preview on a phone is hard. Hard budgets:

| Operation | Budget | Notes |
|---|---|---|
| UI frame (timeline scroll) | < 8 ms | Target 120fps on ProMotion / high-refresh Android |
| Preview frame render | < 33 ms @ 30fps preview | Drop to 540p when budget is tight |
| Cached frame fetch | < 5 ms | LRU GPU texture cache |
| Cold seek + decode | < 200 ms | Acceptable user-perceived lag |
| Project load (10-min timeline) | < 1 s | Lazy load thumbnails and waveforms |
| Export | hardware-encoder bound | Minimize CPU work in the loop |

### Thermal management

Phones throttle hard under sustained GPU load. Detect and degrade gracefully:

- **iOS**: `ProcessInfo.processInfo.thermalState` (`nominal`, `fair`, `serious`, `critical`).
- **Android**: `PowerManager.getCurrentThermalStatus()` (`THERMAL_STATUS_NONE` through `THERMAL_STATUS_SHUTDOWN`).

When state is `serious` or `critical`:

- Drop preview to 540p
- Skip every other frame in preview
- Reduce LUT precision (8-bit instead of 16-bit float)
- Show a non-blocking banner: "Reducing preview quality to keep things smooth"

### Memory

A single 4K frame is ~33 MB uncompressed RGBA8 (66 MB at RGBA16F). Pool aggressively:

- Reuse GPU textures via a pool keyed by `(size, format)`
- Release everything not currently on screen
- On iOS, use `kCVPixelBufferIOSurfaceCoreAnimationCompatibilityKey` so the system can page out under pressure
- Subscribe to memory warnings; flush caches when low

### Power

Background export must be possible but bounded. Use `BGProcessingTask` (iOS) and `WorkManager` (Android). Show a notification with progress; allow the user to cancel.

---

## 16. Security and Privacy

### Privacy by default

- **Never upload user video** without explicit, granular consent. Default = off.
- If cloud features are added (sync, AI captions): be loud and clear about what leaves the device. Show a per-feature data-flow disclosure.
- **iOS**: use `PHPickerViewController` for photo library access — it does not require photo permission, the user picks what they want via Apple's secure UI.
- **Android**: use the **Photo Picker** API for the same reason. Avoid `READ_MEDIA_VIDEO` permission unless absolutely required.

### Secure storage

- Project files are stored in app sandbox; no extra encryption needed unless products require it.
- If the app gains user accounts, store credentials in **Keychain** (iOS) / **EncryptedSharedPreferences** with the **Keystore** (Android).
- Tokens for any third-party APIs: never log them, never include in crash reports.

### Code-side hardening

- Validate all imported files (LUTs, project JSON, video metadata). A malformed `.cube` should produce a clean error, not a crash or memory bug.
- Use `std::filesystem` boundary checks; reject path-traversal attempts in preset packs.
- Set `ProGuard` / R8 rules on Android; enable bitcode and `-Os` on iOS.
- Pin TLS certificates if any network features ship.

### Crash reporting

- **Sentry** or **Firebase Crashlytics**. Strip PII from breadcrumbs.
- Symbolicate dSYMs/native symbols in CI, never on developer machines.

### App Store and Play Store policies

- The folder-LUT-import feature is fine on both stores — `.cube` files are data, not executable code. No sideloading of plugins.
- If you add any AI feature using on-device ML, declare it in the app metadata (Apple now asks).
- Comply with both stores' privacy nutrition labels.

---

## 17. Development Environment, Emulators, Tooling

### Hardware

**Required for any video work:**

- **iOS**: a Mac (Apple Silicon strongly preferred — M2 or newer).
- **Physical iOS device** for performance testing — the simulator does not represent real GPU/codec behavior. Recommended: iPhone 12 (low end of supported devices) and an iPhone 15 Pro or newer (high end).
- **Android**: any modern desktop (Linux/macOS/Windows). Android Studio runs everywhere.
- **Physical Android devices**: a Pixel 6a or Samsung mid-range (low end) and a Pixel 9 Pro / Galaxy S24 Ultra (high end).

### IDEs

- **Xcode 15+** for iOS UI and overall iOS build.
- **Android Studio Hedgehog or newer** for the Android UI.
- **CLion** or **VS Code with CMake Tools + clangd** for the C++ core. CLion has the best CMake integration for cross-platform C++. VS Code is fine if you configure clangd properly.

### Emulators and simulators — what they're good for

| Tool | Good for | Not good for |
|---|---|---|
| **iOS Simulator** | UI iteration, layout, basic logic | GPU performance, codec testing, real-world frame rates, thermal behavior |
| **Android Emulator (AVD)** | UI iteration, API-level testing | Hardware codec testing (some codec features differ from real devices), GPU performance |
| **Xcode device farm / TestFlight** | Distribution to testers, real devices | Doesn't substitute for owning a device |
| **Firebase Test Lab** | Running on a fleet of real Android devices in CI | Cost adds up; use for release validation |
| **AWS Device Farm / BrowserStack** | Cross-device testing matrix | Same as above |

**Critical rule for this app: do all video performance work on physical devices.** Simulators lie. The iOS Simulator on Apple Silicon uses the Mac's GPU, which is dramatically more powerful than any iPhone. The Android emulator uses your desktop GPU. Either will mislead you about real performance.

### Recommended local setup

**iOS:**
- Mac with Apple Silicon (M2 or newer)
- Xcode 15+, command-line tools
- Homebrew + `cmake`, `ninja`, `clang-format`, `swiftformat`, `swiftlint`
- A pair of physical iPhones (one old, one new)

**Android:**
- Android Studio Hedgehog+
- Android SDK platforms 28 through 34
- NDK r26+
- Command-line: `cmake`, `ninja`, `ktlint`, `detekt`
- A pair of physical Android phones (one mid-range, one flagship)

**Cross-platform shared:**
- CLion or VS Code with C++ extensions
- Conan or vcpkg for dependency management
- Docker (optional, for reproducible CI builds)
- Git with LFS for sample media files in the repo

### Debugging tools

- **iOS GPU**: Xcode Frame Debugger and Metal System Trace. Both are excellent.
- **Android GPU**: **RenderDoc** (Vulkan), Android GPU Inspector (AGI), Perfetto for system traces.
- **Codec issues**: `mp4info`, `ffprobe`, `mediainfo` CLIs for inspecting source files.
- **Memory**: Xcode Instruments → Allocations & Leaks; Android Studio Profiler → Memory.
- **Network** (if applicable): Charles Proxy, Proxyman.

### Asset / sample library

Maintain a curated set of test clips covering edge cases:

- 1080p H.264 30fps (baseline)
- 4K H.265 60fps
- 4K HDR HLG
- Variable-frame-rate iPhone footage
- DJI Mavic D-Log clip (10-bit H.265)
- Sony S-Log3 clip
- Vertical 9:16 phone footage
- Footage with audio at -3 dBFS (loud), -30 dBFS (quiet)
- 120fps slow-motion source

Store under `test_assets/` with Git LFS.

---

## 18. Testing Strategy

### Why standard testing isn't enough

You can't unit-test "does the timeline work" with traditional asserts. Media testing has its own discipline.

### Test layers

| Layer | What | Tools |
|---|---|---|
| **Unit (engine)** | Project model, Time arithmetic, command stack, LUT parser | GoogleTest |
| **Pixel comparison** | Render graph output matches golden images within SSIM threshold | Custom harness in C++ |
| **Frame accuracy** | Asking for frame N returns frame N at every framerate | Synthetic colored-frame source |
| **Round-trip** | Render to file → re-import → render again → compare | End-to-end test |
| **Memory** | Open/edit/close project 100x; memory plateaus | Xcode/Android profilers in CI |
| **Performance** | Specific operations meet budgets on reference devices | Firebase Test Lab + custom benchmarks |
| **Integration** | Full export under all preset configurations | Test Lab |
| **UI** | Basic flows work on each platform | XCUITest, Espresso |
| **Manual** | Subjective quality (color accuracy, audio sync) | Human QA |

### Pixel comparison example

```cpp
TEST(LutNode, DLog_To_Rec709_Matches_Golden) {
    auto input = loadTestImage("dlog_test_pattern.png");
    auto lut = parseCube("test_assets/luts/DJI_DLog_to_Rec709.cube");
    
    LutNode node(lut, /*intensity*/ 1.0f);
    RenderContext ctx(MockGpuBackend::create());
    auto output = node.render(ctx, {input});
    
    auto golden = loadTestImage("test_assets/golden/dlog_to_rec709_output.png");
    auto ssim = computeSSIM(output, golden);
    EXPECT_GT(ssim, 0.995);  // tolerate minor float precision differences
}
```

Use SSIM (Structural Similarity), not exact pixel match, because float precision differs across GPU vendors.

### Device matrix for release validation

Mandatory before any release:

- Oldest supported iPhone (iPhone XR / 11 if iOS 16 minimum)
- Newest iPhone (current Pro model)
- Mid-range Android (Pixel 7a, Galaxy A54)
- Flagship Android (Pixel 9 Pro, Galaxy S24 Ultra)
- Tablet (iPad, optionally a Galaxy Tab)

Run the full integration suite on each. Block release on regressions.

---

## 19. CI/CD and Release Engineering

### CI: GitHub Actions (or GitLab CI)

Pipeline structure:

```
push to feature branch
  ├─ Lint (clang-format, swiftlint, ktlint, detekt)
  ├─ Build core C++ (macOS + Linux runners)
  ├─ Unit tests for core (GoogleTest)
  ├─ Build iOS app (Xcode runner)
  ├─ Build Android app (Linux runner with NDK)
  └─ Pixel-comparison tests (CPU rasterizer mock)

PR to main
  ├─ Everything above
  ├─ UI tests (XCUITest in Xcode runner, Espresso in Android runner)
  ├─ Integration tests on Firebase Test Lab (one device per platform)
  └─ Memory leak smoke test

Release branch
  ├─ Everything above
  ├─ Full device matrix on Test Lab
  ├─ Performance benchmarks (block on regression > 10%)
  ├─ Sign + archive iOS (.ipa) → upload to TestFlight
  └─ Sign + bundle Android (.aab) → upload to Play Console internal track
```

### Release cadence

- **Internal builds**: every commit to `main` → TestFlight + Play internal track.
- **Beta releases**: weekly, to a closed group of testers.
- **Production releases**: every 2–4 weeks, with phased rollout (Android: 5% → 25% → 100%; iOS: phased release feature).

### Code signing

- iOS: managed via **fastlane match** (encrypted certs in a private Git repo).
- Android: keystore stored in CI secrets, signing key separate from upload key.

### Versioning

Semantic versioning for the app: `MAJOR.MINOR.PATCH`. Schema versions for project files are independent integers (no semver).

### Feature flags

Use a simple feature flag system from day one, even before remote configuration. Example:

```cpp
namespace features {
    constexpr bool MULTI_TRACK = true;
    constexpr bool OPTICAL_FLOW_RETIME = false;  // not yet stable
    constexpr bool HDR_EXPORT = false;
}
```

Later, swap for a remote service (Firebase Remote Config, LaunchDarkly, Unleash) when needed.

---

## 20. Engineering Best Practices

### Code style

- **C++**: clang-format with Google or LLVM style, `clang-tidy` with a project-defined ruleset. Treat warnings as errors in CI.
- **Swift**: SwiftFormat + SwiftLint, Apple's API Design Guidelines.
- **Kotlin**: ktlint + detekt, Kotlin official style guide.
- **Shaders**: pick GLSL or WGSL as source of truth, generate MSL via SPIRV-Cross at build time. Lint with `glslangValidator`.

### Architecture practices

- **Clean Architecture / Hexagonal** in the core: domain has zero framework dependencies. Application services orchestrate use cases. Platform code is at the edge.
- **Dependency injection** for testability. Even a simple constructor-injected design beats globals.
- **No singletons** in the engine. Pass context explicitly. Singletons hide test seams.
- **Command pattern** for every state mutation. This is what enables undo/redo and replay-based debugging.
- **Immutable value objects** where possible (`Time`, `Resolution`, `ColorSpace`). Reduces aliasing bugs.
- **Document Architecture Decision Records (ADRs)** for every significant choice — color space defaults, codec strategy, FFI shape. Markdown files in `/docs/adr/`. When you join a project six months later, ADRs save weeks.
- **Threading model documented and enforced**. The engine has explicit threads (UI, decode, render, export); cross-thread access goes through clearly marked queues. No "lock and hope."

### Code review

- **Two reviewers** for changes touching the core engine. One for UI-only changes.
- **No merging your own PR**, even with two approvals.
- Reviewer checklist: tests added/updated, performance impact considered, threading model respected, no platform divergence introduced.

### Documentation

- README with one-command build for each platform.
- `docs/architecture.md` (this document).
- `docs/adr/NNNN-title.md` for each major decision.
- `docs/threading.md` describing the threading model.
- `docs/color.md` describing the color pipeline.
- API doc-comments on all public C++/Swift/Kotlin APIs (Doxygen / DocC / KDoc).

### Observability

- Structured logging (`spdlog` in the engine, `OSLog` on iOS, `Timber` on Android).
- Performance counters for frame render time, decode time, encoder throughput. Surface in a developer-mode HUD.
- Crash reporting (Sentry/Crashlytics) with breadcrumbs, scrubbed of PII.
- Optional analytics — opt-in only, document exactly what is collected.

### Security practices

- Treat all user-supplied files (LUTs, video, project JSON) as untrusted. Validate, fuzz-test parsers (use libFuzzer for the C++ parsers).
- Run static analysis: `clang-static-analyzer`, `cppcheck`, Swift's built-in analyzer, Android Lint.
- Run sanitizers in debug builds: ASan, UBSan, TSan rotations.
- Pin dependency versions; review supply chain via `npm audit` / Conan lockfiles / SBOM tooling.

### Performance practices

- Establish performance budgets up front. Measure in CI; fail builds on regression.
- Profile early, profile often. Flame graphs > guesses.
- Cache aggressively but bound caches with LRU + memory pressure listeners.
- Avoid CPU↔GPU round-trips. Stay on the GPU end-to-end during render.
- Batch GPU work; minimize state changes.

### Don't do these things

- **Don't ship FFmpeg early**. It adds 10 MB+, draws battery, and Apple/Google hardware codecs are better. Add it in v2 only if real users need exotic format support.
- **Don't write your own codecs**. Use platform hardware. This is the single biggest mistake teams make in this space.
- **Don't bake effects into proxies or thumbnails**. They are caches; the EDL is the truth.
- **Don't conflate display-referred and scene-referred color**. Pick a working space and stick to it.
- **Don't use float for time**. Rational only.
- **Don't put singletons in the engine**. Test pain compounds.
- **Don't skip ADRs**. Future-you will be grateful.

---

## 21. Phased Roadmap

### Phase 0 — Foundations (4 weeks)

- Project model + JSON serialization + schema versioning
- Time/Rational utility + Command stack with undo/redo
- GPU abstraction skeleton (Metal + Vulkan backends, simple ops working)
- Decode pipeline: hardware decode on both platforms, single-clip preview playing
- CI baseline: build + unit tests on both platforms

**Exit criteria:** A single clip can be loaded and played at native fps on both iOS and Android. Project file can be saved, reloaded, byte-identical.

### Phase 1 — MVP Editor (8 weeks)

- Timeline UI: drag, trim, snap, magnetic mode
- Cut, split, delete, ripple-delete operations
- Color adjustments (linear-correct exposure, contrast, highlights, shadows, saturation)
- Single LUT slot per clip (parser + GPU pipeline)
- Hardcoded export presets: Instagram Reel, YouTube 1080p
- Auto-save + crash recovery

**Exit criteria:** A user can import a clip, trim it, apply a LUT, and export to Instagram Reel format. End-to-end product loop closed.

### Phase 2 — LUT and Preset System (4 weeks)

- Folder watcher for LUT directory on both platforms
- LUT registry with hot-reload
- LUT stacking with intensity per slot
- Project-level LUT (applies after all clips composited)
- Preset packs (JSON manifest + bundled LUTs)
- "Pro Mode" toggle (free-placement timeline)

**Exit criteria:** A user can drop a preset pack folder into the LUT directory and it appears as a one-tap preset in the app, working on both platforms.

### Phase 3 — Pro Features (8 weeks)

- Multi-track timeline (picture-in-picture, overlays)
- Speed ramping per clip
- Transitions: cross-dissolve, dip-to-black, wipe (minimum)
- Custom export with full parameter control (resolution, fps, bitrate, codec, color space)
- Frame-rate conversion (frame blending; optical flow as feature flag)
- Sharpening and full color tools (whites, blacks, vibrance, temperature, tint)

**Exit criteria:** Feature-complete for the spec. Ready for closed beta.

### Phase 4 — Polish, Audio, Release (4+ weeks ongoing)

- Audio: tracks, fades, ducking, waveforms, real-time meters, loudness normalization
- Text and title overlays
- Stickers, drawings (optional)
- Background export with notifications
- HDR end-to-end (Rec.2020 PQ/HLG)
- Performance pass on low-end devices
- Accessibility pass (VoiceOver, TalkBack)
- App Store and Play Store submission

**Exit criteria:** Public 1.0 release.

### Post-1.0 ideas

- Cloud project sync
- AI captions / transcript
- AI scene detection / auto-cut on beat
- AI rotoscoping (mask subjects)
- Shared LUT marketplace
- Multi-cam editing
- Keyframe animation for parameters

### Total estimate

- **MVP (end of Phase 1):** 3 months
- **Beta-ready (end of Phase 3):** 6 months
- **1.0 (end of Phase 4):** 7–8 months

This assumes a small team: 1 senior architect, 2 mobile engineers (one strong iOS, one strong Android, both able to touch C++), 1 graphics/video specialist, 1 QA engineer (joining at Phase 2). Add a designer for at least 50% time from Phase 1.

---

## 22. Risks, Critiques, and Forward-Thinking Notes

### Architectural risks

- **GPU portability** between Metal and Vulkan is real work. Budget time for shader debugging on both. SPIRV-Cross is good but not perfect. **Mitigation:** start with the most complex shader first; if it works on both platforms, the rest will follow.
- **Codec edge cases** are infinite. Variable-frame-rate iPhone footage, malformed MP4s from random apps, HDR metadata mismatches. **Mitigation:** maintain a curated "weird files" test corpus and add to it whenever a real user file fails.
- **Color management complexity**. HDR, wide gamut, multiple log formats — easy to get wrong subtly. **Mitigation:** adopt OCIO from day one even if the v1 features don't seem to need it.
- **Thermal throttling** can make demos great and real-world use bad. **Mitigation:** budget 2 weeks of "long-session" testing before each release.

### Product / strategic notes

- **The wedge**: the LUT-pack workflow you described is genuinely underserved on mobile. CapCut is consumer-friendly but its color tools are weak. LumaFusion has real color tools but a steep learning curve. Lean into the "drop your LUT pack and stack them" workflow as a differentiator. Consider partnering with well-known colorists for launch packs.
- **Audio matters more than you think**. Users will ship videos with terrible audio levels. A simple loudness-normalize-to-target-LUFS toggle on export is high-value-low-effort.
- **OCIO from day one** is overkill for v1 but saves enormous pain at v3 when HDR is added. Strongly recommended.
- **AI features** are tempting but expensive to support well on-device. Defer until v2; ship a great deterministic editor first.

### Things that look easy and aren't

- **Frame-accurate seeking** in long-GOP H.265 with B-frames. Always harder than expected.
- **Audio-video sync** during scrubbing and after speed changes.
- **Project schema migrations** as the model evolves over years.
- **Memory pressure** on 4K timelines on phones with 6 GB RAM.

### Things that look hard and aren't

- **Cross-platform GPU** with SPIRV-Cross is well-trodden ground.
- **The render graph** is a textbook DAG with caching; no novel research needed.
- **The folder-watching preset system** is straightforward on both platforms.

### Final recommendations

1. **Build the engine, not the UI, first.** A polished UI on a broken engine is worthless. A rough UI on a great engine ships.
2. **Pick OCIO and stick with it.** Color management is the area where amateurs and professionals diverge most visibly.
3. **Test on real devices from day one.** Simulators lie about performance.
4. **Audio is not optional.** Plan for it from Phase 1.
5. **Document decisions as ADRs.** Six months from now, you will not remember why you chose Vulkan over GLES. Write it down.
6. **Ship the MVP narrow and deep.** One color science done right beats ten effects done shallow. The "drop a LUT pack and grade your phone footage like a pro" wedge is enough to launch on.

---

*End of document.*
