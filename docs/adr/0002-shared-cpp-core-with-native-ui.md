# 0002 — Shared C++ core with native UI shells

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `core`, `cross-platform`

## Context

We need feature parity across iOS and Android for a professional video editor. The engine touches GPU shaders (Metal / Vulkan), hardware codecs (AVFoundation / MediaCodec), color management, and a non-trivial domain model. Any divergence between the two platforms in editor logic is a defect.

## Decision

A single **C++17/20 core** library implements the entire engine: project model, timeline, render graph, color pipeline, codec orchestration, LUT engine, serialization. iOS (Swift/SwiftUI) and Android (Kotlin/Compose) provide thin native UI shells that call into the core via a stable C ABI.

Platform-specific concerns (Metal vs Vulkan, AVFoundation vs MediaCodec, file pickers, document providers) live behind interfaces in `core/src/platform/` with concrete implementations selected at build time.

## Alternatives considered

- **Flutter / React Native.** Add framework overhead in the GPU and codec paths. Wrappers around AVFoundation/MediaCodec exist but are leaky when frame-accurate seeking and zero-copy textures are required. Rejected for this product class.
- **Kotlin Multiplatform.** Credible for the data layer; insufficient for GPU shaders and codec integration. We'd still need C++, so KMP just adds a third language.
- **Rust core.** Safer pointer/lifetime story but most video libraries (FFmpeg, OpenColorIO, libplacebo) are C/C++; Apple/Google sample code is C++ first; mobile graphics hiring is C++-deep. Defensible if the team has Rust expertise but a higher-friction default.
- **Two parallel native engines.** Guarantees divergence within months. Not viable.

## Consequences

- **Positive:** Feature parity is structural, not aspirational. Color science is provably identical. CI can run pixel-comparison tests in one place.
- **Positive:** Hire one engine team, two thin UI teams.
- **Negative:** C++ build complexity (CMake + Gradle + Xcode integration) is non-trivial. Mitigated by careful build setup and `docs/build.md`.
- **Negative:** FFI boundary requires discipline (see [ADR-0006](./0006-clean-hexagonal-architecture.md) and `docs/ffi.md`).

## References

- `docs/architecture.md` §3, §5
- `docs/ffi.md`
