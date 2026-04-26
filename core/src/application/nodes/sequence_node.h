#ifndef VX_APPLICATION_SEQUENCE_NODE_H
#define VX_APPLICATION_SEQUENCE_NODE_H

#include <vector>
#include <mutex>

#include "application/nodes/render_node.h"
#include "domain/clip.h"

namespace vx {

/**
 * @brief Represents a clip with its associated media source path.
 */
struct ResolvedClip {
    Clip clip;
    std::string source_path;
};

/**
 * @brief Manages a list of clips and selects the active one for rendering.
 */
class SequenceNode : public RenderNode {
public:
    SequenceNode();
    ~SequenceNode() override;

    void setClips(std::vector<ResolvedClip> clips);

    TextureHandle render(IGpuBackend& backend, ICodecBackend& codec_backend, Time time, std::span<const TextureHandle> inputs) override;

private:
    std::vector<ResolvedClip> clips_;
    mutable std::mutex clips_mutex_;
};

}  // namespace vx

#endif  // VX_APPLICATION_SEQUENCE_NODE_H
