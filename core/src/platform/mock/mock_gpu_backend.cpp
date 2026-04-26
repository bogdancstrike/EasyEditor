#include "mock_gpu_backend.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "util/error.h"

namespace vx {

namespace {

struct MockTexture {
    Size2D size;
    PixelFormat format = PixelFormat::RGBA16F;
    std::vector<uint8_t> bytes;
};

[[nodiscard]] size_t checkedPixelCount(Size2D size) {
    if (size.width <= 0 || size.height <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "texture dimensions must be positive");
    }
    return static_cast<size_t>(size.width) * static_cast<size_t>(size.height);
}

[[nodiscard]] size_t bytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA8_UNORM:
        case PixelFormat::BGRA8_UNORM:
            return 4;
        case PixelFormat::RGBA16F:
            return 8;
    }
    throw Error(VX_ERR_INVALID_ARG, "unsupported pixel format");
}

[[nodiscard]] MockTexture& requireMockTexture(TextureHandle texture) {
    if (!texture.valid()) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle is invalid");
    }
    return *static_cast<MockTexture*>(texture.opaque);
}

void validateHandleMatchesTexture(TextureHandle handle, const MockTexture& texture) {
    if (handle.size.width != texture.size.width || handle.size.height != texture.size.height ||
        handle.format != texture.format) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle metadata does not match storage");
    }
}

}  // namespace

TextureHandle MockGpuBackend::allocateTexture(Size2D size, PixelFormat fmt) {
    const size_t byte_count = checkedPixelCount(size) * bytesPerPixel(fmt);
    auto texture = std::make_unique<MockTexture>();
    texture->size = size;
    texture->format = fmt;
    texture->bytes.resize(byte_count);

    TextureHandle handle;
    handle.opaque = texture.release();
    handle.size = size;
    handle.format = fmt;
    return handle;
}

void MockGpuBackend::releaseTexture(TextureHandle texture) {
    delete static_cast<MockTexture*>(texture.opaque);
}

void MockGpuBackend::dispatchCompute(std::string_view shader_name,
                                     std::span<const TextureHandle> /*inputs*/,
                                     TextureHandle /*output*/,
                                     const ShaderConstants& /*constants*/) {
    throw Error(VX_ERR_UNSUPPORTED,
                "MockGpuBackend has no compute shader implementation: " + std::string{shader_name});
}

void MockGpuBackend::copyTexture(TextureHandle src, TextureHandle dst) {
    const MockTexture& source = requireMockTexture(src);
    MockTexture& destination = requireMockTexture(dst);
    validateHandleMatchesTexture(src, source);
    validateHandleMatchesTexture(dst, destination);

    if (source.size.width != destination.size.width || source.size.height != destination.size.height ||
        source.format != destination.format) {
        throw Error(VX_ERR_INVALID_ARG, "copyTexture requires matching texture size and format");
    }

    destination.bytes = source.bytes;
}

std::span<uint8_t> mockTextureBytes(TextureHandle texture) {
    MockTexture& mock = requireMockTexture(texture);
    validateHandleMatchesTexture(texture, mock);
    return mock.bytes;
}

std::span<const uint8_t> mockTextureBytesConst(TextureHandle texture) {
    const MockTexture& mock = requireMockTexture(texture);
    validateHandleMatchesTexture(texture, mock);
    return mock.bytes;
}

double computeSsim(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    if (a.size() != b.size() || a.empty()) {
        throw Error(VX_ERR_INVALID_ARG, "SSIM inputs must be non-empty and equal-sized");
    }

    double mean_a = 0.0;
    double mean_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        mean_a += static_cast<double>(a[i]);
        mean_b += static_cast<double>(b[i]);
    }
    mean_a /= static_cast<double>(a.size());
    mean_b /= static_cast<double>(b.size());

    double variance_a = 0.0;
    double variance_b = 0.0;
    double covariance = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double da = static_cast<double>(a[i]) - mean_a;
        const double db = static_cast<double>(b[i]) - mean_b;
        variance_a += da * da;
        variance_b += db * db;
        covariance += da * db;
    }

    const double count = static_cast<double>(a.size());
    variance_a /= count;
    variance_b /= count;
    covariance /= count;

    constexpr double kL = 255.0;
    constexpr double kC1 = (0.01 * kL) * (0.01 * kL);
    constexpr double kC2 = (0.03 * kL) * (0.03 * kL);

    const double numerator = (2.0 * mean_a * mean_b + kC1) * (2.0 * covariance + kC2);
    const double denominator =
        (mean_a * mean_a + mean_b * mean_b + kC1) * (variance_a + variance_b + kC2);

    if (denominator == 0.0) {
        return 1.0;
    }
    return numerator / denominator;
}

}  // namespace vx
