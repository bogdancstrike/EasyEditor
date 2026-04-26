#include "project_service.h"

#include <chrono>
#include <memory>
#include <string>

#include "util/error.h"

namespace vx {

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

}  // namespace vx
