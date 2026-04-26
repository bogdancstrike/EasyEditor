#include "gles_backend.h"

#include <memory>

#include "util/error.h"
#include "util/log.h"

#ifdef ANDROID
#include <GLES3/gl32.h>
#endif

namespace vx {

namespace {

const char* const kLut3dFragmentShader = R"(#version 320 es
precision mediump float;
precision mediump sampler3D;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sTexture;
layout(binding = 1) uniform sampler3D sLut;

in vec2 vTexCoord;

void main() {
    vec4 color = texture(sTexture, vTexCoord);
    // Apply 3D LUT. color.rgb acts as 0.0-1.0 coordinates in the 33x33x33 cube.
    vec3 graded = texture(sLut, color.rgb).rgb;
    outColor = vec4(graded, color.a);
}
)";

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
    unsigned int target = 0; // GL_TEXTURE_2D, GL_TEXTURE_3D, etc.
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
    texture->target = GL_TEXTURE_2D;

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

TextureHandle GlesBackend::allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> data) {
#ifdef ANDROID
    if (size <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "texture dimensions must be positive");
    }

    auto texture = std::make_unique<GlesTexture>();
    texture->size = {size, size}; // For 3D we reuse Size2D for WxH, depth is implicitly 'size'
    texture->format = fmt;
    texture->target = GL_TEXTURE_3D;

    unsigned int raw_id = 0;
    glGenTextures(1, &raw_id);
    texture->id = GlTextureId(raw_id);

    glBindTexture(GL_TEXTURE_3D, texture->id.get());

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
        default:
            throw Error(VX_ERR_INVALID_ARG, "unsupported pixel format for 3D texture");
    }

    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, size, size, size, 0, format, type, data.data());

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);

    TextureHandle handle;
    handle.opaque = texture.release();
    handle.size = {size, size};
    handle.format = fmt;
    return handle;
#else
    static_cast<void>(size);
    static_cast<void>(fmt);
    static_cast<void>(data);
    throw Error(VX_ERR_UNSUPPORTED, "GlesBackend only supported on Android");
#endif
}

void GlesBackend::updateTexture3D(TextureHandle texture, std::span<const uint8_t> data) {
#ifdef ANDROID
    auto& gles_texture = requireTexture(texture);
    validateTextureMetadata(texture, gles_texture);

    if (gles_texture.target != GL_TEXTURE_3D) {
        throw Error(VX_ERR_INVALID_ARG, "texture is not a 3D texture");
    }

    glBindTexture(GL_TEXTURE_3D, gles_texture.id.get());

    GLenum format;
    GLenum type;

    switch (gles_texture.format) {
        case PixelFormat::RGBA8_UNORM:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case PixelFormat::RGBA16F:
            format = GL_RGBA;
            type = GL_HALF_FLOAT;
            break;
        default:
            throw Error(VX_ERR_INVALID_ARG, "unsupported pixel format for 3D texture update");
    }

    int s = texture.size.width; // We use WxH to store size in allocateTexture3D
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, s, s, s, format, type, data.data());

    glBindTexture(GL_TEXTURE_3D, 0);
#else
    static_cast<void>(texture);
    static_cast<void>(data);
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
                                 std::span<const TextureHandle> inputs,
                                 TextureHandle output,
                                 const ShaderConstants& constants) {
    if (shader_name == "lut_3d") {
        VX_LOG_INFO("GlesBackend", "dispatchCompute: lut_3d (applying 3D LUT via fragment shader)");
        // In a real implementation, this would:
        // 1. Compile/cache kLut3dFragmentShader (and a full-screen quad vertex shader).
        // 2. Set up a Framebuffer (FBO) with 'output' as the color attachment.
        // 3. Bind inputs[0] to unit 0 (sampler2D) and inputs[1] to unit 1 (sampler3D).
        // 4. Draw a full-screen quad.
        // For Phase 0, we just log and verify the shader source is present.
        return;
    }
    VX_LOG_INFO("GlesBackend", (std::string{"dispatchCompute: "} + std::string{shader_name}).c_str());
}

void GlesBackend::copyTexture(TextureHandle src, TextureHandle dst) {
    static_cast<void>(src);
    static_cast<void>(dst);
    throw Error(VX_ERR_UNSUPPORTED, "GlesBackend::copyTexture not implemented");
}

void GlesBackend::present(TextureHandle texture, void* window) {
#ifdef ANDROID
    static_cast<void>(texture);
    static_cast<void>(window);
    // TODO: EGL surface management and blit output texture to window.
    VX_LOG_INFO("GlesBackend", "present requested for window");
#else
    static_cast<void>(texture);
    static_cast<void>(window);
#endif
}

void GlesBackend::waitForGpu() {
#ifdef ANDROID
    glFinish();
#endif
}

}  // namespace vx
