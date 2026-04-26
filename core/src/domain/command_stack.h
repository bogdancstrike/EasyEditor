#ifndef VX_DOMAIN_COMMAND_STACK_H
#define VX_DOMAIN_COMMAND_STACK_H

#include <cstddef>
#include <memory>
#include <vector>

#include "command.h"

namespace vx {

struct Project;

/// Undo/redo stack. THE single point through which Project mutates.
/// See ADR-0008.
class CommandStack {
public:
    static constexpr size_t kDefaultMaxDepth = 200;

    explicit CommandStack(size_t max_depth = kDefaultMaxDepth);

    /// Apply a command and push it to the undo stack. Clears redo.
    /// Throws if cmd->apply throws (state is unchanged in that case).
    void execute(std::unique_ptr<Command> cmd, Project& project);

    [[nodiscard]] bool canUndo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redo_.empty(); }

    void undo(Project& project);
    void redo(Project& project);

    void clear() noexcept;

    [[nodiscard]] size_t undoDepth() const noexcept { return undo_.size(); }
    [[nodiscard]] size_t redoDepth() const noexcept { return redo_.size(); }

private:
    size_t max_depth_;
    std::vector<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
};

}  // namespace vx

#endif  // VX_DOMAIN_COMMAND_STACK_H
