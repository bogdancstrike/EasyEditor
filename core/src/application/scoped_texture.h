#ifndef VX_APPLICATION_SCOPED_TEXTURE_H
#define VX_APPLICATION_SCOPED_TEXTURE_H

#include "platform/i_gpu_backend.h"

namespace vx {

/**
 * @brief RAII helper for TextureHandle.
 *
 * Automatically releases the texture via the provided IGpuBackend in its destructor.
 */
class ScopedTexture {
public:
    ScopedTexture(IGpuBackend& backend, TextureHandle handle)
        : backend_(backend), handle_(handle) {}

    ~ScopedTexture() {
        if (handle_.valid()) {
            backend_.releaseTexture(handle_);
        }
    }

    // Disable copy
    ScopedTexture(const ScopedTexture&) = delete;
    ScopedTexture& operator=(const ScopedTexture&) = delete;

    // Enable move
    ScopedTexture(ScopedTexture&& other) noexcept
        : backend_(other.backend_), handle_(other.handle_) {
        other.handle_ = {};
    }

    ScopedTexture& operator=(ScopedTexture&& other) noexcept {
        if (this != &other) {
            if (handle_.valid()) {
                backend_.releaseTexture(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = {};
        }
        return *this;
    }

    [[nodiscard]] TextureHandle get() const { return handle_; }
    [[nodiscard]] TextureHandle release() {
        TextureHandle h = handle_;
        handle_ = {};
        return h;
    }

    [[nodiscard]] operator TextureHandle() const { return handle_; }

private:
    IGpuBackend& backend_;
    TextureHandle handle_;
};

}  // namespace vx

#endif  // VX_APPLICATION_SCOPED_TEXTURE_H
