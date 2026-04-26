#ifndef VX_APPLICATION_PROJECT_SERVICE_H
#define VX_APPLICATION_PROJECT_SERVICE_H

#include <memory>
#include <string>

#include "domain/project.h"

namespace vx {

class IGpuBackend;
class ICodecBackend;
class RenderGraph;

/// Application-level project use cases exposed to the FFI layer.
///
/// This service owns orchestration only. Domain defaults and serialization stay
/// in `domain/project.*`; FFI ownership and error translation stay in `ffi/`.
class ProjectService {
public:
    ProjectService();
    ~ProjectService();

    [[nodiscard]] std::unique_ptr<Project> createProject(const std::string& name) const;
    [[nodiscard]] std::string serializeJson(const Project& project) const;
    [[nodiscard]] std::unique_ptr<Project> loadJson(const std::string& json) const;

    /**
     * @brief Adds a media asset to the project and automatically creates a clip in the first sequence.
     * 
     * This is a helper for Phase 1 PoC to simplify the UX. In a full implementation,
     * adding to the pool and adding to the timeline would be separate operations.
     */
    void addMediaAsset(Project& project, const std::string& path, Time duration, Resolution resolution) const;

    /**
     * @brief Renders a frame of the project's first sequence to the given native window.
     */
    void renderFrame(Project& project, void* window, Time time) const;

private:
    std::unique_ptr<IGpuBackend> gpu_backend_;
    std::unique_ptr<ICodecBackend> codec_backend_;
    mutable std::unique_ptr<RenderGraph> render_graph_;
};

}  // namespace vx

#endif  // VX_APPLICATION_PROJECT_SERVICE_H
