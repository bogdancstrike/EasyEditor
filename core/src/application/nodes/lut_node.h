#ifndef VX_APPLICATION_LUT_NODE_H
#define VX_APPLICATION_LUT_NODE_H

#include <vector>
#include <cstdint>

#include "application/nodes/render_node.h"

namespace vx {

/**
 * @brief Applies a 3D LUT to the input texture.
 *
 * Hardcoded to 33x33x33 for Phase 1.
 */
class LutNode : public RenderNode {
public:
    explicit LutNode(IGpuBackend& backend, std::span<const uint8_t> lut_data);
    ~LutNode() override;

    void setLutData(std::span<const uint8_t> lut_data);

    TextureHandle render(IGpuBackend& backend, ICodecBackend& codec_backend, Time time, std::span<const TextureHandle> inputs) override;

private:
    IGpuBackend& backend_;
    TextureHandle lut_texture_;
    TextureHandle cached_output_;
};

}  // namespace vx

#endif  // VX_APPLICATION_LUT_NODE_H
