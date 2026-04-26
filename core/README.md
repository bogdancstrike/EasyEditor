# core/

Shared C++20 engine. Consumed by the Android app via JNI and (Phase 5) by iOS via Swift's C interop.

## Layout

```
core/
├── CMakeLists.txt          Top-level build for the engine library and tests.
├── include/vx/             Public headers (FFI surface). Stable C ABI.
│   ├── public_api.h        The complete C surface Swift/Kotlin bind to.
│   └── version.h           Build-time version constants.
├── src/
│   ├── domain/             Pure logic: project model, time, render-graph nodes,
│   │                       color math. NO platform headers, NO services.
│   ├── application/        Use-case orchestration: ProjectService, ImportService,
│   │                       ExportService, EffectsService, PresetService.
│   │                       Depends on domain and on platform interfaces only.
│   ├── platform/           Interfaces (IGpuBackend, ICodecBackend, IFileSystemBackend)
│   │                       and concrete implementations under platform/<backend>/.
│   ├── ffi/                C ABI implementation. Translates exceptions to status codes,
│   │                       wraps services in opaque handles. The ONLY layer that uses
│   │                       extern "C".
│   └── util/               Logging shim, error types, IDs.
├── shaders/                GLSL compute shaders (source of truth) → compiled to SPIR-V at build.
├── tests/                  GoogleTest suites mirroring src/ layout.
└── third_party/            Pinned dependencies (CMake FetchContent or vendored).
```

## Layering rules

These are **enforced** by `tools/scripts/check_layering.py` in CI.

- `domain/**` includes nothing outside `domain/` and `util/`.
- `application/**` may include `domain/`, `platform/` interfaces, `util/`. **Never** platform implementations or `ffi/`.
- `platform/**/*.h` (interfaces) include only `domain/` and `util/`. Concrete implementations under `platform/<backend>/` may include their backend's SDK.
- `ffi/**` includes everything; nothing includes `ffi/`.

If a layering rule blocks legitimate work, that is a signal to write an ADR proposing the change — not to suppress the check.

## Building standalone

See `docs/build.md`. TL;DR for desktop tests:

```bash
cmake -S .. -B ../build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build ../build
ctest --test-dir ../build --output-on-failure
```

The Android Gradle build invokes the same `core/CMakeLists.txt` per ABI.

## Adding a public function

See `docs/ffi.md` for the full procedure.
