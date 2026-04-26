#include "sequence_node.h"

#include <algorithm>

namespace vx {

SequenceNode::SequenceNode() = default;
SequenceNode::~SequenceNode() = default;

void SequenceNode::setClips(std::vector<Clip> clips) {
    std::lock_guard<std::mutex> lock(clips_mutex_);
    clips_ = std::move(clips);
}

TextureHandle SequenceNode::render(IGpuBackend& backend, Time time, std::span<const TextureHandle> /*inputs*/) {
    // Find if any clip is active at 'time'
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(clips_mutex_);
        auto it = std::find_if(clips_.begin(), clips_.end(), [time](const Clip& clip) {
            return time >= clip.timeline_start && time < (clip.timeline_start + clip.duration);
        });
        found = (it != clips_.end());
    }

    // In Phase 1, we always return a texture. If no clip is active, it might be black.
    Size2D size{1920, 1080};
    auto output = backend.allocateTexture(size, PixelFormat::RGBA16F);

    if (found) {
        // We found a clip. In a real implementation, we'd trigger decoding.
        ShaderConstants constants;
        backend.dispatchCompute("source_placeholder", {}, output, constants);
    } else {
        // No clip active: render black
        backend.dispatchCompute("clear_black", {}, output, {});
    }

    return output;
}

}  // namespace vx
