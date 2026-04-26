#ifndef VX_DOMAIN_PROJECT_MIGRATION_H
#define VX_DOMAIN_PROJECT_MIGRATION_H

#include <string>

namespace vx {

/// Migrate a project JSON document to the current project schema.
///
/// Phase 0 only supports schema v1, so this currently validates the schema and
/// returns the input unchanged. The function exists now so v2 can be added
/// without changing callers.
[[nodiscard]] std::string migrateProjectJsonToCurrent(const std::string& json);

}  // namespace vx

#endif  // VX_DOMAIN_PROJECT_MIGRATION_H
