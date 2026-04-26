#include "sequence_node.h"

#include <algorithm>

namespace vx {

SequenceNode::SequenceNode() = default;
SequenceNode::~SequenceNode() = default;

void SequenceNode::setClips(std::vector<ResolvedClip> clips) {
    std::lock_guard<std::mutex> lock(clips_mutex_);
    clips_ = std::move(clips);
}

TextureHandle SequenceNode::render(IGpuBackend& backend, ICodecBackend& codec_backend, Time time, std::span<const TextureHandle> /*inputs*/) {
    // Find if any clip is active at 'time'
    bool found = false;
    std::string active_source_path;
    Time source_time;

    {
        std::lock_guard<std::mutex> lock(clips_mutex_);
        auto it = std::find_if(clips_.begin(), clips_.end(), [time](const ResolvedClip& clip) {
            return time >= clip.clip.timeline_start && time < (clip.clip.timeline_start + clip.clip.duration);
        });
        if (it != clips_.end()) {
            found = true;
            active_source_path = it->source_path;
            // Calculate source time based on timeline time
            Time offset_in_clip = time - it->clip.timeline_start;
            source_time = it->clip.source_in + offset_in_clip;
        }
    }

    if (found) {
        // Use codec_backend to decode the frame at calculated source time
        int64_t time_us = static_cast<int64_t>(source_time.toSeconds() * 1'000'000);
        return codec_backend.getFrameAtTime(active_source_path, time_us);
    } else {
        // No clip active: render black
        Size2D size{1920, 1080};
        auto output = backend.allocateTexture(size, PixelFormat::RGBA16F);
        backend.dispatchCompute("clear_black", {}, output, {});
        return output;
    }
}

}  // namespace vx
