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

}  // namespace vx
