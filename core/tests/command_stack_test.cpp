#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "domain/command.h"
#include "domain/command_stack.h"
#include "domain/project.h"

namespace {

class RenameProjectCommand final : public vx::Command {
public:
    explicit RenameProjectCommand(std::string next) : next_(std::move(next)) {}

    void apply(vx::Project& project) override {
        previous_ = project.name;
        project.name = next_;
    }

    void revert(vx::Project& project) noexcept override {
        project.name = previous_;
    }

    [[nodiscard]] std::string description() const override {
        return "Rename Project";
    }

private:
    std::string next_;
    std::string previous_;
};

void test_execute_undo_redo() {
    vx::Project project;
    project.name = "Before";

    vx::CommandStack stack{2};
    stack.execute(std::make_unique<RenameProjectCommand>("After"), project);

    assert(project.name == "After");
    assert(stack.canUndo());
    assert(!stack.canRedo());

    stack.undo(project);
    assert(project.name == "Before");
    assert(!stack.canUndo());
    assert(stack.canRedo());

    stack.redo(project);
    assert(project.name == "After");
    assert(stack.canUndo());
    assert(!stack.canRedo());
}

void test_bounded_depth() {
    vx::Project project;
    vx::CommandStack stack{1};

    stack.execute(std::make_unique<RenameProjectCommand>("One"), project);
    stack.execute(std::make_unique<RenameProjectCommand>("Two"), project);

    assert(stack.undoDepth() == 1);
    stack.undo(project);
    assert(project.name == "One");
}

}  // namespace

int main() {
    test_execute_undo_redo();
    test_bounded_depth();
    return 0;
}
