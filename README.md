# Video Editor App

Android-first mobile video editor for short-form video workflows. The product goal is an editor where a user can import clips, choose which media belongs on the timeline, scrub and preview the edit live, apply LUT-based looks, and export the finished sequence back to the Android Gallery.

The long-term architecture is a shared C++ engine with native UI shells. Android is the active implementation; iOS is planned for a later phase.

## Current Phase

**Phase 1 — MVP Editor** is in progress.

Phase 1 is considered done when this flow works end to end:

1. Import at least two videos from the Android Gallery.
2. Keep imported media in a media pool; only selected media is added to the timeline.
3. Preview the sequenced timeline live.
4. Scrub forward/backward by dragging the timeline cursor.
5. Apply a LUT to the whole project.
6. Apply a LUT override to an individual timeline clip.
7. Preview the video with LUTs applied.
8. Export the result to Android MediaStore/Gallery.

## Features

Implemented or partially implemented:

- Android app built with Kotlin and Jetpack Compose.
- Shared C++ core for project model, timeline domain objects, render graph skeleton, and JNI/FFI entry points.
- Android Photo Picker import.
- Separate media pool and timeline state in the Android UI.
- Timeline clip selection and tap/drag scrubbing.
- GLES preview path with 3D LUT sampling.
- Built-in LUTs:
  - DLog to Rec709
  - Daylight
  - Daylight Sharp
  - Night
  - Warm
  - Cool
  - Cinematic
  - Orange & Teal
  - Faded Film
- Export skeleton using Android `MediaExtractor`, `MediaCodec`, `MediaMuxer`, and `MediaStore`.

Still active:

- Render-graph-backed export with LUTs applied.
- Drag-and-drop media placement from media pool to timeline.
- Trim handles, snapping, and richer timeline feedback.
- Full project persistence and restore from Android UI.

## Repository Layout

```text
.
├── android/           Android app: Kotlin, Jetpack Compose, JNI, Gradle
├── core/              Shared C++ engine: domain, app services, render graph, platform interfaces
├── docs/              Architecture, ADRs, build notes, threading, color, TODO
├── design-system/     Product/design notes
└── build/             Local build outputs, including copied APKs
```

## Architecture

The app follows the decisions in `docs/architecture.md` and `docs/adr/`:

- **Shared C++ core, native UI shells.** Android uses Kotlin/Compose. iOS will be added later against the same C ABI.
- **Clean / Hexagonal architecture.** Domain code is platform-free. Platform-specific GPU/codec work lives behind interfaces.
- **Non-destructive editing.** The project references media assets and timeline clips; source files are not modified.
- **Rational time.** Timeline math uses exact time types in the core.
- **Color pipeline discipline.** The target pipeline is linear, scene-referred, LUT-ready rendering.

## Build Prerequisites

Minimum local tooling:

- Java 17 or newer
- Android SDK with API 34 installed
- Android NDK r26 or newer
- CMake 3.22+
- Ninja

The checked-in Gradle wrapper is used for Android builds.

## Build The APK

From the repository root:

```bash
cd android
./gradlew :app:assembleDebug
```

The debug APK is produced at:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

To copy it into the repo-level APK output folder used during development:

```bash
mkdir -p ../build/apk
cp app/build/outputs/apk/debug/app-debug.apk ../build/apk/video-editor-debug.apk
```

## Install On Emulator Or Device

If `adb` is on `PATH`:

```bash
cd android
./gradlew :app:installDebug
```

Or install the copied APK directly:

```bash
adb install -r build/apk/video-editor-debug.apk
adb shell am start -n com.videoeditor.app/.MainActivity
```

If `adb` is not on `PATH`, use the SDK path directly, for example:

```bash
$HOME/Android/Sdk/platform-tools/adb install -r build/apk/video-editor-debug.apk
```

## Core Tests

The C++ core can be built and tested outside Android:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVX_BUILD_TESTS=ON \
  -DVX_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Documentation

- [docs/TODO.md](docs/TODO.md) — current milestone tasks and completion log
- [docs/architecture.md](docs/architecture.md) — product and technical architecture
- [docs/build.md](docs/build.md) — build system details
- [docs/color.md](docs/color.md) — color pipeline decisions
- [docs/ffi.md](docs/ffi.md) — C ABI and JNI/Swift binding conventions
- [docs/threading.md](docs/threading.md) — threading model
- [docs/coding-standards.md](docs/coding-standards.md) — coding and review rules
- [docs/adr/](docs/adr/) — architecture decision records

## Development Notes

- Update `docs/TODO.md` whenever a Phase task is completed or the next task changes.
- Keep Android-specific details out of `core/src/domain`.
- Add or update an ADR for significant architecture decisions.
- Prefer testing UI changes on the running emulator with `adb` after `assembleDebug`.
