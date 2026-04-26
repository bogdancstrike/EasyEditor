#include "project.h"
#include "project_serialization_internal.h"

#include <chrono>
#include <string>
#include <vector>

#include "util/error.h"
#include "vx/version.h"

namespace vx {

// Custom serialization for Project to handle chrono time_points and schema versioning
void to_json(json& j, const Project& p) {
    j = json{
        {"schema_version", p.schema_version},
        {"id", p.id},
        {"name", p.name},
        {"created_at_ms", timePointToMs(p.created_at)},
        {"modified_at_ms", timePointToMs(p.modified_at)},
        {"canvas", p.canvas},
        {"framerate", p.framerate},
        {"color_space", p.color_space},
        {"media_pool", p.media_pool},
        {"sequences", p.sequences}
    };
}

void from_json(const json& j, Project& p) {
    j.at("schema_version").get_to(p.schema_version);
    if (p.schema_version > VX_SCHEMA_VERSION_CURRENT) {
        throw Error(VX_ERR_UNSUPPORTED, "project schema version is newer than this engine");
    }

    j.at("id").get_to(p.id);
    j.at("name").get_to(p.name);
    
    p.created_at = msToTimePoint(j.at("created_at_ms").get<int64_t>());
    p.modified_at = msToTimePoint(j.at("modified_at_ms").get<int64_t>());

    j.at("canvas").get_to(p.canvas);
    j.at("framerate").get_to(p.framerate);
    j.at("color_space").get_to(p.color_space);
    
    if (j.contains("media_pool")) {
        j.at("media_pool").get_to(p.media_pool);
    }
    if (j.contains("sequences")) {
        j.at("sequences").get_to(p.sequences);
    }
}

std::string projectToJson(const Project& p) {
    return json(p).dump();
}

Project projectFromJson(const std::string& input) {
    try {
        return json::parse(input).get<Project>();
    } catch (const json::exception& e) {
        throw Error(VX_ERR_INVALID_ARG, std::string("JSON parse error: ") + e.what());
    }
}

}  // namespace vx
