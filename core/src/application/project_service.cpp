#include "project_service.h"

#include <chrono>
#include <memory>
#include <string>

#include "util/error.h"
#include "util/log.h"
#include "application/render_graph.h"

#ifdef ANDROID
#include "platform/gles/gles_backend.h"
#include "platform/android/android_codec_backend.h"
#else
#include "platform/mock/mock_gpu_backend.h"
#include "platform/mock/mock_codec_backend.h"
#endif

namespace vx {

ProjectService::ProjectService() {
#ifdef ANDROID
    gpu_backend_ = std::make_unique<GlesBackend>();
    codec_backend_ = std::make_unique<AndroidCodecBackend>();
#else
    gpu_backend_ = std::make_unique<MockGpuBackend>();
    codec_backend_ = std::make_unique<MockCodecBackend>();
#endif
}

ProjectService::~ProjectService() = default;

std::unique_ptr<Project> ProjectService::createProject(const std::string& name) const {
    if (name.empty()) {
        throw Error(VX_ERR_INVALID_ARG, "project name cannot be empty");
    }

    auto project = std::make_unique<Project>();
    project->id = Uuid::generate();
    project->name = name;
    const auto now = std::chrono::system_clock::now();
    project->created_at = now;
    project->modified_at = now;
    return project;
}

std::string ProjectService::serializeJson(const Project& project) const {
    return projectToJson(project);
}

std::unique_ptr<Project> ProjectService::loadJson(const std::string& json) const {
    return std::make_unique<Project>(projectFromJson(json));
}

void ProjectService::addMediaAsset(Project& project, const std::string& path, Time duration, Resolution resolution) const {
    // 1. Create and add MediaAsset
    MediaAsset asset;
    asset.id = Uuid::generate();
    asset.source_path = path;
    asset.duration = duration;
    asset.resolution = resolution;
    project.media_pool.push_back(asset);

    // 2. Ensure at least one sequence and one track exists
    if (project.sequences.empty()) {
        Sequence seq;
        seq.id = Uuid::generate();
        project.sequences.push_back(seq);
    }
    
    auto& sequence = project.sequences.front();
    if (sequence.tracks.empty()) {
        Track track;
        track.id = Uuid::generate();
        sequence.tracks.push_back(track);
    }

    // 3. Create a Clip and add it to the first track
    auto& track = sequence.tracks.front();
    
    // Calculate start time based on existing clips
    Time start = Time::zero();
    for (const auto& clip : track.clips) {
        Time clip_end = clip.timeline_start + clip.duration;
        if (start < clip_end) {
            start = clip_end;
        }
    }

    Clip clip;
    clip.id = Uuid::generate();
    clip.asset_id = asset.id;
    clip.timeline_start = start;
    clip.source_in = Time::zero();
    clip.duration = duration;
    
    track.clips.push_back(clip);
    
    project.modified_at = std::chrono::system_clock::now();
}

void ProjectService::renderFrame(Project& project, void* window, Time time) const {
    static_cast<void>(window); // TODO: Window management
    
    if (!render_graph_) {
        render_graph_ = std::make_unique<RenderGraph>(*gpu_backend_);
    }

    // Update render graph with current sequence
    if (!project.sequences.empty()) {
        const auto& sequence = project.sequences.front();
        std::vector<ResolvedClip> resolved_clips;
        for (const auto& track : sequence.tracks) {
            for (const auto& clip : track.clips) {
                // Find asset path
                std::string path;
                for (const auto& asset : project.media_pool) {
                    if (asset.id == clip.asset_id) {
                        path = asset.source_path;
                        break;
                    }
                }
                if (!path.empty()) {
                    resolved_clips.push_back({clip, path});
                }
            }
        }
        render_graph_->setClips(resolved_clips);
    }

    // Execute render graph
    TextureHandle final_frame = render_graph_->execute(*codec_backend_, time);
    
    gpu_backend_->present(final_frame, window);
}

}  // namespace vx
