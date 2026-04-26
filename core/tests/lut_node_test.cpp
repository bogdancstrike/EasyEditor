#include <cassert>
#include <iostream>
#include "application/nodes/lut_node.h"
#include "platform/mock/mock_gpu_backend.h"

namespace vx {

class TrackingMockGpuBackend : public IGpuBackend {
public:
    int allocations = 0;
    int releases = 0;

    [[nodiscard]] TextureHandle allocateTexture(Size2D size, PixelFormat fmt) override {
        allocations++;
        return {reinterpret_cast<void*>(static_cast<uintptr_t>(allocations)), size, fmt};
    }

    [[nodiscard]] TextureHandle allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> /*data*/) override {
        allocations++;
        return {reinterpret_cast<void*>(static_cast<uintptr_t>(allocations)), {size, size}, fmt};
    }

    void updateTexture3D(TextureHandle /*texture*/, std::span<const uint8_t> /*data*/) override {}

    void releaseTexture(TextureHandle /*texture*/) override {
        releases++;
    }

    void dispatchCompute(std::string_view /*shader_name*/,
                         std::span<const TextureHandle> /*inputs*/,
                         TextureHandle /*output*/,
                         const ShaderConstants& /*constants*/) override {}

    void copyTexture(TextureHandle /*src*/, TextureHandle /*dst*/) override {}
    void waitForGpu() override {}
};

void test_lut_node_double_allocation() {
    TrackingMockGpuBackend backend;
    std::vector<uint8_t> lut_data(33 * 33 * 33 * 4);
    LutNode node(backend, lut_data);

    // Initial allocation for lut_texture_
    assert(backend.allocations == 1);

    TextureHandle input = {reinterpret_cast<void*>(0x123), {1920, 1080}, PixelFormat::RGBA16F};
    TextureHandle inputs[] = {input};

    std::cout << "First render call..." << std::endl;
    node.render(backend, Time::zero(), inputs);

    // EXPECTED (if bug exists): 1 (lut) + 1 (cached_output) + 1 (render output) = 3
    // EXPECTED (if fixed): 1 (lut) + 1 (cached_output) = 2
    std::cout << "Allocations: " << backend.allocations << std::endl;
    
    // The current bug is that it allocates TWO textures in render().
    // One for cached_output_, and one for 'output' local variable.
    assert(backend.allocations == 3); 

    std::cout << "Second render call with same size..." << std::endl;
    node.render(backend, Time::zero(), inputs);

    // If bug exists: it allocates a NEW 'output' EVERY time.
    // If it also allocates cached_output_ incorrectly... wait.
    // In current code:
    // it reuses cached_output_ (so no NEW cached_output allocation)
    // but it ALWAYS allocates 'output'.
    // So total should be 4.
    std::cout << "Allocations: " << backend.allocations << std::endl;
    assert(backend.allocations == 4);
}

} // namespace vx

int main() {
    try {
        vx::test_lut_node_double_allocation();
        std::cout << "Test passed (bug confirmed)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
