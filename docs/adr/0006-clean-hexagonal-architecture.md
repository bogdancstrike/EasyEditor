# 0006 — Clean / Hexagonal architecture for the core

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `core`, `architecture`

## Context

The engine spans non-trivial domain logic (project model, render graph, color science) and platform-specific concerns (GPU, codec, file system). Without explicit layering, the domain code grows tendrils into platform headers, and the engine becomes impossible to unit-test off-device or to port. Both of these matter: ([ADR-0003](./0003-android-first-then-ios.md)) we will port to iOS later, and we want to test pixel-correctness on a CI Linux runner.

## Decision

The core uses **Clean / Hexagonal architecture** with these layers:

```
core/src/
  domain/       Pure logic. Project model, time, render-graph nodes, color math.
                NO platform headers. NO services. NO I/O.
  application/  Use-case orchestration (ImportService, ExportService, ProjectService).
                Depends on domain and on platform interfaces (not implementations).
  platform/     Interfaces: IGpuBackend, ICodecBackend, IFileSystemBackend.
                Concrete impls under platform/<backend>/, picked by build flags.
  ffi/          C ABI exposing application services to Swift/Kotlin.
  util/         Leaf utilities: logging shim, error types, IDs.
```

Dependencies flow inward only: `ffi → application → domain` and `platform interfaces ← application`. Concrete platform implementations are wired in at startup (composition root in `ffi/init.cpp`).

This is enforced by:

- A CI script (`tools/scripts/check_layering.py`) that scans `#include` directives and fails on cross-layer violations.
- Per-directory `CMakeLists.txt` that exposes only the headers a layer is allowed to see.

## Alternatives considered

- **Loose layering / no enforcement.** Inevitably becomes spaghetti. We've seen it in every codebase that didn't have automated checks.
- **Microkernel with plugin DLLs.** Overkill; we don't need runtime-loaded plugins.
- **A single flat namespace.** Easy now, painful at scale and impossible to test off-device.

## Consequences

- **Positive:** Domain code is unit-testable on Linux/macOS without an Android device. The render graph runs against a `MockGpuBackend` software rasterizer.
- **Positive:** Adding the iOS platform is "implement three interfaces" rather than "rewrite the engine."
- **Positive:** New contributors can navigate the codebase without a tour.
- **Negative:** A small amount of friction when adding a feature that legitimately spans layers — must be split into the right files. Acceptable cost.

## References

- `docs/architecture.md` §5
- `docs/coding-standards.md` § "Layering rules"
- Alistair Cockburn, "Hexagonal Architecture" (a.k.a. Ports and Adapters)
