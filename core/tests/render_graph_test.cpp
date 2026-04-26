#include <cassert>
#include "application/render_graph.h"
#include "platform/mock/mock_gpu_backend.h"

namespace vx {

class MockCodecBackend : public ICodecBackend {
public:
    [[nodiscard]] bool openMedia(const std::string&) override { return true; }
    TextureHandle getFrameAtTime(const std::string&, int64_t) override {
        return TextureHandle{0, {1920, 1080}, PixelFormat::RGBA16F};
    }
    [[nodiscard]] std::unique_ptr<IDecoder> openDecoder(const std::string&) override {
        return nullptr;
    }
    [[nodiscard]] std::unique_ptr<IEncoder> openEncoder(const std::string&, const IEncoder::Settings&) override {
        return nullptr;
    }
};

void test_render_graph_execution() {
    MockGpuBackend backend;
    MockCodecBackend codec;
    RenderGraph graph(backend);
    
    TextureHandle handle = graph.execute(codec, Time::zero());
    assert(handle.valid());
    assert(handle.format == PixelFormat::RGBA16F);
}

} // namespace vx

int main() {
    vx::test_render_graph_execution();
    return 0;
}
