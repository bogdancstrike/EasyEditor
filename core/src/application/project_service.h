#ifndef VX_APPLICATION_PROJECT_SERVICE_H
#define VX_APPLICATION_PROJECT_SERVICE_H

#include <memory>
#include <string>

#include "domain/project.h"

namespace vx {

/// Application-level project use cases exposed to the FFI layer.
///
/// This service owns orchestration only. Domain defaults and serialization stay
/// in `domain/project.*`; FFI ownership and error translation stay in `ffi/`.
class ProjectService {
public:
    [[nodiscard]] std::unique_ptr<Project> createProject(const std::string& name) const;
    [[nodiscard]] std::string serializeJson(const Project& project) const;
    [[nodiscard]] std::unique_ptr<Project> loadJson(const std::string& json) const;
};

}  // namespace vx

#endif  // VX_APPLICATION_PROJECT_SERVICE_H
