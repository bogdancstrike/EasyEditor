#include "gles_backend.h"

#include <memory>

#include "util/error.h"
#include "util/log.h"

#ifdef ANDROID
#include <GLES3/gl32.h>
#endif

namespace vx {

namespace {

/// RAII wrapper for OpenGL texture IDs.
struct GlTextureId {
    unsigned int id = 0;

    GlTextureId() = default;
    explicit GlTextureId(unsigned int i) : id(i) {}

    ~GlTextureId() {
        reset();
    }

    void reset() {
        if (id != 0) {
#ifdef ANDROID
            glDeleteTextures(1, &id);
#endif
            id = 0;
        }
    }

    GlTextureId(const GlTextureId&) = delete;
    GlTextureId& operator=(const GlTextureId&) = delete;

    GlTextureId(GlTextureId&& other) noexcept : id(other.id) {
        other.id = 0;
    }

    GlTextureId& operator=(GlTextureId&& other) noexcept {
        if (this != &other) {
            reset();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    [[nodiscard]] unsigned int get() const { return id; }
};

struct GlesTexture {
    GlTextureId id;
    Size2D size;
    PixelFormat format = PixelFormat::RGBA16F;
};

[[nodiscard]] GlesTexture& requireTexture(TextureHandle texture) {
    if (!texture.valid()) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle is invalid");
    }
    return *static_cast<GlesTexture*>(texture.opaque);
}

void validateTextureMetadata(TextureHandle handle, const GlesTexture& texture) {
    if (handle.size.width != texture.size.width || handle.size.height != texture.size.height ||
        handle.format != texture.format) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle metadata does not match GLES texture");
    }
}

}  // namespace

GlesBackend::GlesBackend() {
    VX_LOG_INFO("GlesBackend", "GlesBackend initialized");
}

GlesBackend::~GlesBackend() = default;

TextureHandle GlesBackend::allocateTexture(Size2D size, PixelFormat fmt) {
#ifdef ANDROID
    if (size.width <= 0 || size.height <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "texture dimensions must be positive");
    }

    auto texture = std::make_unique<GlesTexture>();
    texture->size = size;
    texture->format = fmt;

    unsigned int raw_id = 0;
    glGenTextures(1, &raw_id);
    texture->id = GlTextureId(raw_id);

    glBindTexture(GL_TEXTURE_2D, texture->id.get());

    GLint internal_format;
    GLenum format;
    GLenum type;

    switch (fmt) {
        case PixelFormat::RGBA8_UNORM:
            internal_format = GL_RGBA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case PixelFormat::RGBA16F:
            internal_format = GL_RGBA16F;
            format = GL_RGBA;
            type = GL_HALF_FLOAT;
            break;
        case PixelFormat::BGRA8_UNORM:
            // GL_BGRA_EXT is 0x80E1. In GLES, BGRA is often only for the external format,
            // while internal format remains GL_RGBA8.
            internal_format = GL_RGBA8;
            format = 0x80E1;  // GL_BGRA_EXT
            type = GL_UNSIGNED_BYTE;
            break;
        default:
            throw Error(VX_ERR_INVALID_ARG, "unsupported pixel format");
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, size.width, size.height, 0, format, type, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    TextureHandle handle;
    handle.opaque = texture.release();
    handle.size = size;
    handle.format = fmt;
    return handle;
#else
    static_cast<void>(size);
    static_cast<void>(fmt);
    throw Error(VX_ERR_UNSUPPORTED, "GlesBackend only supported on Android");
#endif
}

void GlesBackend::releaseTexture(TextureHandle texture) {
    if (texture.opaque == nullptr) {
        return;
    }
    auto* gles_texture = static_cast<GlesTexture*>(texture.opaque);
    validateTextureMetadata(texture, *gles_texture);
    delete gles_texture;
}

void GlesBackend::dispatchCompute(std::string_view shader_name,
                                 std::span<const TextureHandle> /*inputs*/,
                                 TextureHandle /*output*/,
                                 const ShaderConstants& /*constants*/) {
    VX_LOG_INFO("GlesBackend", (std::string{"dispatchCompute: "} + std::string{shader_name}).c_str());
}

void GlesBackend::copyTexture(TextureHandle src, TextureHandle dst) {
    static_cast<void>(src);
    static_cast<void>(dst);
    throw Error(VX_ERR_UNSUPPORTED, "GlesBackend::copyTexture not implemented");
}

void GlesBackend::waitForGpu() {
#ifdef ANDROID
    glFinish();
#endif
}

}  // namespace vx
