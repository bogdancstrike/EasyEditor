#include "command_stack.h"

#include "project.h"

namespace vx {

CommandStack::CommandStack(size_t max_depth) : max_depth_(max_depth) {}

void CommandStack::execute(std::unique_ptr<Command> cmd, Project& project) {
    cmd->apply(project);  // may throw; if so, stack unchanged
    redo_.clear();
    undo_.push_back(std::move(cmd));
    if (undo_.size() > max_depth_) {
        undo_.erase(undo_.begin());
    }
}

void CommandStack::undo(Project& project) {
    if (undo_.empty()) return;
    auto cmd = std::move(undo_.back());
    undo_.pop_back();
    cmd->revert(project);
    redo_.push_back(std::move(cmd));
}

void CommandStack::redo(Project& project) {
    if (redo_.empty()) return;
    auto cmd = std::move(redo_.back());
    redo_.pop_back();
    cmd->apply(project);
    undo_.push_back(std::move(cmd));
}

void CommandStack::clear() noexcept {
    undo_.clear();
    redo_.clear();
}

}  // namespace vx
