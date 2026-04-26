#ifndef VX_PLATFORM_MOCK_MOCK_GPU_BACKEND_H
#define VX_PLATFORM_MOCK_MOCK_GPU_BACKEND_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "platform/i_gpu_backend.h"

namespace vx {

/// CPU-backed GPU backend for off-device render graph tests.
///
/// TextureHandle::opaque points to an internal MockTexture allocated by this
/// backend. Handles must be released through the same backend instance.
class MockGpuBackend final : public IGpuBackend {
public:
    [[nodiscard]] TextureHandle allocateTexture(Size2D size, PixelFormat fmt) override;
    [[nodiscard]] TextureHandle allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> data) override;
    void updateTexture3D(TextureHandle texture, std::span<const uint8_t> data) override;
    void releaseTexture(TextureHandle texture) override;

    void dispatchCompute(std::string_view shader_name,
                         std::span<const TextureHandle> inputs,
                         TextureHandle output,
                         const ShaderConstants& constants) override;

    void copyTexture(TextureHandle src, TextureHandle dst) override;
    void waitForGpu() override {}
};

/// Mutable pixel storage for tests that need to seed source textures.
[[nodiscard]] std::span<uint8_t> mockTextureBytes(TextureHandle texture);

/// Read-only pixel storage for assertions and comparison.
[[nodiscard]] std::span<const uint8_t> mockTextureBytesConst(TextureHandle texture);

/// Structural Similarity Index for equal-sized byte buffers.
///
/// Returns 1.0 for identical buffers and approaches 0.0 as buffers diverge.
/// This byte-level helper is deliberately format-agnostic; render tests can
/// choose which channels to feed into it.
[[nodiscard]] double computeSsim(std::span<const uint8_t> a, std::span<const uint8_t> b);

}  // namespace vx

#endif  // VX_PLATFORM_MOCK_MOCK_GPU_BACKEND_H
