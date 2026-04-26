#ifndef VX_DOMAIN_COMMAND_H
#define VX_DOMAIN_COMMAND_H

#include <string>

namespace vx {

struct Project;

/// Base of all state mutations. See ADR-0008.
///
/// Rules:
///   - apply() and revert() must be exact inverses.
///   - revert() MUST succeed if apply() succeeded — never throw from revert.
///   - Commands store the minimum delta needed to revert (not full snapshots).
///   - description() is the human-readable label shown in the Undo menu
///     ("Trim", "Add Clip", "Change Exposure"). Keep it short.
class Command {
public:
    virtual ~Command() = default;
    virtual void apply(Project& project) = 0;
    virtual void revert(Project& project) noexcept = 0;
    [[nodiscard]] virtual std::string description() const = 0;
};

}  // namespace vx

#endif  // VX_DOMAIN_COMMAND_H
