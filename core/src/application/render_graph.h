#ifndef VX_APPLICATION_RENDER_GRAPH_H
#define VX_APPLICATION_RENDER_GRAPH_H

#include <memory>
#include <vector>

#include "domain/time.h"
#include "platform/i_gpu_backend.h"
#include "application/nodes/sequence_node.h"
#include "application/nodes/lut_node.h"

namespace vx {

/// Orchestrates the execution of render nodes to produce a final frame.
///
/// Phase 1 scope: basic structure to hold nodes and execute them sequentially.
class RenderGraph {
public:
    explicit RenderGraph(IGpuBackend& backend);
    ~RenderGraph();

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    /// Executes the graph for the given time and returns the final texture.
    /// Phase 1: returns a placeholder texture.
    [[nodiscard]] TextureHandle execute(ICodecBackend& codec_backend, Time time);

    SequenceNode& sequence_node() { return *sequence_node_; }
    LutNode& lut_node() { return *lut_node_; }

private:
    IGpuBackend& backend_;
    TextureHandle output_placeholder_;

    std::unique_ptr<SequenceNode> sequence_node_;
    std::unique_ptr<LutNode> lut_node_;
};

}  // namespace vx

#endif  // VX_APPLICATION_RENDER_GRAPH_H
