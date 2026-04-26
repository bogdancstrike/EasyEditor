# 0004 — Vulkan primary, OpenGL ES 3.2 fallback (Android)

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `android`, `gpu`

## Context

The render graph requires compute shaders, multi-pass pipelines, and zero-copy import of decoder output. Android offers two paths:

- **Vulkan 1.1+** (mandatory for new devices since Android 7; mature on most chipsets by Android 10).
- **OpenGL ES 3.2** (universal, but driver quality varies and Khronos has effectively frozen the spec).

Vulkan gives us lower CPU overhead, explicit synchronization, and a clean path to AV1 hardware-decode integration via `VK_KHR_external_memory_android_hardware_buffer`. It is also closer to Metal in concept, which will reduce the cost of the iOS port ([ADR-0003](./0003-android-first-then-ios.md)).

OpenGL ES is simpler to start with but a dead-end: declining vendor investment, no path to modern features, requires a separate `GL_TEXTURE_EXTERNAL_OES` integration story that doesn't transfer to iOS.

## Decision (Accepted)

We implement **Vulkan 1.1 as the primary backend** for modern devices. However, because the primary development environment (Android Emulator AVD) frequently fails to initialize Vulkan (`VK_ERROR_INITIALIZATION_FAILED`) and to support older devices with buggy Vulkan drivers, we **must implement an OpenGL ES 3.2 fallback**. 

We will maintain both backends, using Vulkan when available and falling back to OpenGL ES 3.2 when `vkCreateInstance` fails or on known problematic devices. The public interface `IGpuBackend` will hide this complexity from the core render graph.

## Alternatives considered

- **OpenGL ES first, Vulkan later.** Lower initial complexity. Rejected because the iOS port (Metal) maps better from Vulkan than from GLES, and we'd pay the migration cost twice.
- **Vulkan + GLES from day one.** Doubles the GPU code we maintain, doubles shader testing surface, slows every render-graph change. Rejected on cost.
- **ANGLE (GLES on top of Vulkan).** Adds an indirection without solving the fundamental problem.

## Consequences

- **Positive:** One backend to maintain. Smaller binary. Better performance ceiling. Easier path to iOS Metal.
- **Negative:** A small slice of Vulkan-flaky devices (notably some older Adreno 5xx, Mali-T8xx) will be unsupported. We accept this; our target user has a recent device.
- **Negative:** Vulkan boilerplate is heavy. Mitigated by isolating it inside `core/src/platform/vulkan/` with a deliberately small public surface (`IGpuBackend`).

## To resolve before "Accepted"

- [ ] Verify Vulkan device coverage on our target audience using Android Distribution dashboard + crash-reporter device data once we have any.
- [ ] Stand up a Vulkan "hello triangle + compute pass" on a Pixel 6a and a Samsung A-series mid-range. Confirm no driver crashes.
- [ ] Decide minimum Android API: 28 vs 30. (Currently 28.)

## Phase 0 implementation notes

- 2026-04-26: Added `VulkanBackend` skeleton under `core/src/platform/vulkan/` and Android JNI smoke diagnostics. The backend initializes Vulkan, selects a compute queue, allocates one image-backed texture, and submits a no-op command buffer.
- 2026-04-26: Pixel 10 Pro XL 16 KB x86_64 AVD is not a valid acceptance signal for this ADR. The AVD advertises Vulkan features, but `vkCreateInstance` returns `VK_ERROR_INITIALIZATION_FAILED`, Android's `cmd gpu vkjson` returns `{}`, and logcat shows emulator Mesa/virtgpu property access denials. Real Pixel hardware remains required before moving this ADR to Accepted.

## References

- `docs/architecture.md` §4, §7
