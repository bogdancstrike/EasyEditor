#ifndef VX_PLATFORM_GLES_GLES_BACKEND_H
#define VX_PLATFORM_GLES_GLES_BACKEND_H

#include <span>
#include <string>
#include <string_view>

#include "platform/i_gpu_backend.h"

namespace vx {

/// OpenGL ES 3.2 implementation of IGpuBackend.
///
/// Thread affinity: Render thread. Caller must ensure an EGL/GLES context
/// is current before calling any method.
class GlesBackend final : public IGpuBackend {
public:
    GlesBackend();
    ~GlesBackend() override;

    GlesBackend(const GlesBackend&) = delete;
    GlesBackend& operator=(const GlesBackend&) = delete;
    GlesBackend(GlesBackend&&) = delete;
    GlesBackend& operator=(GlesBackend&&) = delete;

    [[nodiscard]] TextureHandle allocateTexture(Size2D size, PixelFormat fmt) override;
    [[nodiscard]] TextureHandle allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> data) override;
    void updateTexture3D(TextureHandle texture, std::span<const uint8_t> data) override;
    void releaseTexture(TextureHandle texture) override;

    void dispatchCompute(std::string_view shader_name,
                         std::span<const TextureHandle> inputs,
                         TextureHandle output,
                         const ShaderConstants& constants) override;

    void copyTexture(TextureHandle src, TextureHandle dst) override;
    void waitForGpu() override;
};

}  // namespace vx

#endif  // VX_PLATFORM_GLES_GLES_BACKEND_H
