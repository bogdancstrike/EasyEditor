#include <algorithm>
#include <cassert>

#include "platform/mock/mock_gpu_backend.h"
#include "util/error.h"

namespace {

void test_allocate_seed_copy_and_release() {
    vx::MockGpuBackend backend;
    const vx::Size2D size{2, 2};
    const vx::TextureHandle src = backend.allocateTexture(size, vx::PixelFormat::RGBA8_UNORM);
    const vx::TextureHandle dst = backend.allocateTexture(size, vx::PixelFormat::RGBA8_UNORM);

    auto src_bytes = vx::mockTextureBytes(src);
    for (size_t i = 0; i < src_bytes.size(); ++i) {
        src_bytes[i] = static_cast<uint8_t>(i);
    }

    backend.copyTexture(src, dst);
    assert(std::ranges::equal(vx::mockTextureBytesConst(src), vx::mockTextureBytesConst(dst)));

    backend.releaseTexture(src);
    backend.releaseTexture(dst);
}

void test_copy_rejects_mismatched_format() {
    vx::MockGpuBackend backend;
    const vx::TextureHandle src = backend.allocateTexture(vx::Size2D{1, 1}, vx::PixelFormat::RGBA8_UNORM);
    const vx::TextureHandle dst = backend.allocateTexture(vx::Size2D{1, 1}, vx::PixelFormat::RGBA16F);

    try {
        backend.copyTexture(src, dst);
        assert(false);
    } catch (const vx::Error& e) {
        assert(e.status() == VX_ERR_INVALID_ARG);
    }

    backend.releaseTexture(src);
    backend.releaseTexture(dst);
}

void test_ssim() {
    constexpr uint8_t a[] = {0, 64, 128, 255};
    constexpr uint8_t b[] = {0, 64, 128, 255};
    constexpr uint8_t c[] = {255, 128, 64, 0};

    const double same = vx::computeSsim(a, b);
    const double different = vx::computeSsim(a, c);

    assert(same == 1.0);
    assert(different < same);
}

void test_allocate_texture_3d() {
    vx::MockGpuBackend backend;
    constexpr int size = 4;
    std::vector<uint8_t> data(size * size * size * 4, 123); // RGBA8_UNORM
    
    const vx::TextureHandle handle = backend.allocateTexture3D(size, vx::PixelFormat::RGBA8_UNORM, data);
    assert(handle.valid());
    assert(handle.size.width == size);
    assert(handle.size.height == size);
    assert(handle.format == vx::PixelFormat::RGBA8_UNORM);

    const auto bytes = vx::mockTextureBytesConst(handle);
    assert(bytes.size() == data.size());
    assert(std::ranges::equal(bytes, data));

    backend.releaseTexture(handle);
}

}  // namespace

int main() {
    test_allocate_seed_copy_and_release();
    test_copy_rejects_mismatched_format();
    test_allocate_texture_3d();
    test_ssim();
    return 0;
}
