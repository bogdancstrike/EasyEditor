#ifndef VX_APPLICATION_RENDER_NODE_H
#define VX_APPLICATION_RENDER_NODE_H

#include <span>

#include "domain/time.h"
#include "platform/i_gpu_backend.h"
#include "platform/i_codec_backend.h"

namespace vx {

/**
 * @brief Base class for all nodes in the render graph.
 *
 * A RenderNode takes a set of input textures and produces an output texture
 * for a specific point in time.
 */
class RenderNode {
public:
    virtual ~RenderNode() = default;

    /**
     * @brief Executes the node's rendering logic.
     *
     * @param backend The GPU backend to use for rendering commands.
     * @param codec_backend The codec backend to use for decoding frames.
     * @param time The current timeline time to render.
     * @param inputs The input textures from parent nodes.
     * @return The resulting texture handle. The caller (RenderGraph) is responsible for lifecycle.
     */
    virtual TextureHandle render(IGpuBackend& backend, ICodecBackend& codec_backend, Time time, std::span<const TextureHandle> inputs) = 0;
};

}  // namespace vx

#endif  // VX_APPLICATION_RENDER_NODE_H
