#ifndef VX_PLATFORM_I_GPU_BACKEND_H
#define VX_PLATFORM_I_GPU_BACKEND_H

#include <cstdint>
#include <span>
#include <string_view>

namespace vx {

enum class PixelFormat {
    RGBA8_UNORM,
    RGBA16F,        // primary intermediate format; see ADR-0007
    BGRA8_UNORM,
};

struct Size2D { int width = 0; int height = 0; };

/// Opaque handle. Backend interprets the void* — callers do not.
struct TextureHandle {
    void* opaque = nullptr;
    Size2D size;
    PixelFormat format = PixelFormat::RGBA16F;

    [[nodiscard]] bool valid() const noexcept { return opaque != nullptr; }
};

/// Per-frame shader inputs. Will be expanded as compute shaders need more types.
struct ShaderConstants {
    // Phase-0 placeholder. Real implementation will be a uniform buffer wrapper.
    void* opaque = nullptr;
};

/// Platform-agnostic GPU surface. Implementations: VulkanBackend (Android),
/// MetalBackend (iOS, Phase 5), MockGpuBackend (tests).
///
/// Thread affinity: all methods MUST be called on the Render thread (or the
/// Export thread for offline rendering). See docs/threading.md.
class IGpuBackend {
public:
    virtual ~IGpuBackend() = default;

    [[nodiscard]] virtual TextureHandle allocateTexture(Size2D size, PixelFormat fmt) = 0;
    virtual void releaseTexture(TextureHandle) = 0;

    virtual void dispatchCompute(std::string_view shader_name,
                                 std::span<const TextureHandle> inputs,
                                 TextureHandle output,
                                 const ShaderConstants& constants) = 0;

    virtual void copyTexture(TextureHandle src, TextureHandle dst) = 0;
    virtual void waitForGpu() = 0;
};

}  // namespace vx

#endif  // VX_PLATFORM_I_GPU_BACKEND_H
