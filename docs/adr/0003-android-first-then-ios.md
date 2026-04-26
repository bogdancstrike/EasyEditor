# 0003 — Ship Android first, iOS in Phase 5

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `roadmap`, `android`, `ios`

## Context

The full roadmap (`docs/architecture.md` §21) targets feature parity on iOS and Android. We are a small team in early stages. Building both shells in parallel triples the surface area we have to keep working, and slows time-to-feedback on the engine. We need to pick a starting platform.

The engine is shared C++ ([ADR-0002](./0002-shared-cpp-core-with-native-ui.md)), so platform choice does not constrain the engine design — provided we keep iOS in mind throughout.

## Decision

We ship **Android only** through Phase 4. iOS work begins in Phase 5, reusing the same core unmodified.

Until iOS work begins, we maintain these "iOS-ready" disciplines:

1. **No Android-only assumptions in `core/`.** GPU access is via `IGpuBackend` (Metal will be a sibling implementation of `VulkanBackend`). Codec access is via `ICodecBackend`. File access is via `IFileSystemBackend`.
2. **No platform-specific types in the FFI.** The C ABI is the same one Swift will bind to.
3. **Shaders are authored in GLSL** and compiled to SPIR-V at build time. SPIRV-Cross will translate to MSL when iOS lands. We do **not** hand-author MSL.
4. **Color science, time arithmetic, project model** are validated by tests that run on Linux/macOS without an Android device — the same tests will run for iOS.

## Alternatives considered

- **iOS first.** Apple's tooling (Metal frame capture, Instruments) is more mature than Android's, which would speed early engine work. But Android's openness, lower-friction sideloading, and (for the user) an Android-first lifestyle make it the better starting point here. Also: Vulkan is the harder GPU API; if we get Vulkan working, Metal is downhill.
- **Both at once.** Triples coordination cost for a small team. Rejected.
- **iOS only forever.** Forecloses the Android market and removes a major pressure-test on the cross-platform abstractions. Rejected.

## Consequences

- **Positive:** Faster MVP. One UI to design, one app store to ship to, one device matrix to test.
- **Positive:** When iOS lands, the engine is already battle-tested.
- **Negative:** No real proof the iOS path works until Phase 5. Mitigated by interface discipline and by occasionally building `core/` on macOS to catch portability regressions early.
- **Negative:** The team's iOS muscle atrophies in the meantime if it exists at all. Plan to add an iOS-experienced engineer ahead of Phase 5.

## References

- `docs/TODO.md` (Phase 5)
- `docs/architecture.md` §3, §21
