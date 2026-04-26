#include "project_migration.h"

#include "project.h"
#include "util/error.h"
#include "vx/version.h"

namespace vx {

std::string migrateProjectJsonToCurrent(const std::string& json) {
    const Project project = projectFromJson(json);
    if (project.schema_version == VX_SCHEMA_VERSION_CURRENT) {
        return json;
    }

    throw Error(VX_ERR_UNSUPPORTED, "project schema migration is not available");
}

}  // namespace vx
