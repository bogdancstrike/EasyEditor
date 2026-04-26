#include "render_graph.h"

namespace vx {

RenderGraph::RenderGraph(IGpuBackend& backend) : backend_(backend) {
    // For now, allocate a small placeholder texture so execute() has something to return.
    output_placeholder_ = backend_.allocateTexture({16, 16}, PixelFormat::RGBA8_UNORM);
}

RenderGraph::~RenderGraph() {
    backend_.releaseTexture(output_placeholder_);
}

TextureHandle RenderGraph::execute(Time /*time*/) {
    // Phase 1: simple no-op execution.
    // In Task 4, this will walk the node tree.
    backend_.dispatchCompute("noop", {}, output_placeholder_, ShaderConstants{});
    return output_placeholder_;
}

}  // namespace vx
