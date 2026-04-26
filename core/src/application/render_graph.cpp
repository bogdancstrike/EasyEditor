#include "render_graph.h"
#include "application/scoped_texture.h"

namespace vx {

RenderGraph::RenderGraph(IGpuBackend& backend) : backend_(backend) {
    // For now, allocate a small placeholder texture so execute() has something to return.
    output_placeholder_ = backend_.allocateTexture({16, 16}, PixelFormat::RGBA16F);

    sequence_node_ = std::make_unique<SequenceNode>();
    
    // Initialize LutNode with identity LUT data for Phase 1
    std::vector<uint8_t> identity_lut(33 * 33 * 33 * 4);
    for (int r = 0; r < 33; ++r) {
        for (int g = 0; g < 33; ++g) {
            for (int b = 0; b < 33; ++b) {
                size_t idx = (r + g * 33 + b * 33 * 33) * 4;
                identity_lut[idx + 0] = static_cast<uint8_t>(r * 255 / 32);
                identity_lut[idx + 1] = static_cast<uint8_t>(g * 255 / 32);
                identity_lut[idx + 2] = static_cast<uint8_t>(b * 255 / 32);
                identity_lut[idx + 3] = 255;
            }
        }
    }
    lut_node_ = std::make_unique<LutNode>(backend_, identity_lut);
}

RenderGraph::~RenderGraph() {
    backend_.releaseTexture(output_placeholder_);
}

TextureHandle RenderGraph::execute(ICodecBackend& codec_backend, Time time) {
    // Chain SequenceNode -> LutNode
    
    // 1. SequenceNode produces the source frame
    // Use ScopedTexture to ensure source_frame is released even if subsequent steps throw.
    ScopedTexture source_frame(backend_, sequence_node_->render(backend_, codec_backend, time, {}));
    
    // 2. LutNode takes the source frame and applies the 3D LUT
    TextureHandle lut_inputs[] = {source_frame.get()};
    TextureHandle final_frame = lut_node_->render(backend_, codec_backend, time, lut_inputs);
    
    // 3. Update output_placeholder (for Phase 1, we just return the final_frame and release old placeholder)
    if (final_frame.opaque != output_placeholder_.opaque) {
        backend_.releaseTexture(output_placeholder_);
        output_placeholder_ = final_frame;
    }

    return output_placeholder_;
}

}  // namespace vx
