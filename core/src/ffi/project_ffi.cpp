#include "vx/public_api.h"

#include <cstring>
#include <memory>
#include <new>
#include <string>

#include "application/project_service.h"
#include "exception_guard.h"

struct VxProject {
    std::unique_ptr<vx::Project> project;
};

namespace {

vx::ProjectService& projectService() {
    static vx::ProjectService service;
    return service;
}

vx_status_t makeString(const std::string& value, vx_string_t* out) {
    if (out == nullptr) {
        return VX_ERR_INVALID_ARG;
    }

    auto storage = std::make_unique<std::string>(value);
    out->data = storage->data();
    out->length = storage->size();
    out->_opaque = storage.release();
    return VX_OK;
}

}  // namespace

extern "C" {

void vx_string_free(vx_string_t* s) {
    if (s == nullptr || s->_opaque == nullptr) {
        return;
    }

    delete static_cast<std::string*>(s->_opaque);
    s->data = nullptr;
    s->length = 0;
    s->_opaque = nullptr;
}

[[nodiscard]] VxProject* vx_project_create(const char* name) {
    if (name == nullptr) {
        return nullptr;
    }

    try {
        auto handle = std::make_unique<VxProject>();
        handle->project = projectService().createProject(name);
        return handle.release();
    } catch (...) {
        return nullptr;
    }
}

void vx_project_destroy(VxProject* project) {
    delete project;
}

vx_status_t vx_project_serialize_json(VxProject* project, vx_string_t* out_json) VX_FFI_TRY {
    if (project == nullptr || project->project == nullptr || out_json == nullptr) {
        return VX_ERR_INVALID_ARG;
    }

    *out_json = vx_string_t{nullptr, 0, nullptr};
    return makeString(projectService().serializeJson(*project->project), out_json);
}
VX_FFI_CATCH

vx_status_t vx_project_load_json(const char* json, VxProject** out_project) VX_FFI_TRY {
    if (json == nullptr || out_project == nullptr) {
        return VX_ERR_INVALID_ARG;
    }

    *out_project = nullptr;
    auto handle = std::make_unique<VxProject>();
    handle->project = projectService().loadJson(json);
    *out_project = handle.release();
    return VX_OK;
}
VX_FFI_CATCH

vx_status_t vx_project_add_asset(VxProject* project, const char* path, int64_t duration_ms, int32_t width, int32_t height) VX_FFI_TRY {
    if (project == nullptr || project->project == nullptr || path == nullptr) {
        return VX_ERR_INVALID_ARG;
    }

    vx::Time duration = vx::Time::fromSeconds(static_cast<double>(duration_ms) / 1000.0);
    vx::Resolution resolution{width, height};
    projectService().addMediaAsset(*project->project, path, duration, resolution);
    return VX_OK;
}
VX_FFI_CATCH

vx_status_t vx_project_render_frame(VxProject* project, void* window, int64_t time_ms) VX_FFI_TRY {
    if (project == nullptr || project->project == nullptr || window == nullptr) {
        return VX_ERR_INVALID_ARG;
    }

    projectService().renderFrame(*project->project, window, vx::Time::fromSeconds(static_cast<double>(time_ms) / 1000.0));
    return VX_OK;
}
VX_FFI_CATCH

}  // extern "C"
