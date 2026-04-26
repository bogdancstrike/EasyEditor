#include <cassert>
#include "application/render_graph.h"
#include "platform/mock/mock_gpu_backend.h"

namespace vx {

void test_render_graph_execution() {
    MockGpuBackend backend;
    RenderGraph graph(backend);
    
    TextureHandle handle = graph.execute(Time::zero());
    assert(handle.valid());
    assert(handle.format == PixelFormat::RGBA16F);
}

} // namespace vx

int main() {
    vx::test_render_graph_execution();
    return 0;
}
