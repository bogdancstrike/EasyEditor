#include <cassert>
#include <chrono>
#include <string>

#include "domain/project.h"
#include "domain/project_migration.h"
#include "util/error.h"

namespace {

vx::Project makeProject() {
    vx::Project project;
    project.id = vx::Uuid::fromString("00112233-4455-6677-8899-aabbccddeeff");
    project.name = "Cut \"A\" \\ Test";
    project.created_at = std::chrono::system_clock::time_point{std::chrono::milliseconds{1234}};
    project.modified_at = std::chrono::system_clock::time_point{std::chrono::milliseconds{5678}};
    project.canvas = vx::Resolution{1080, 1920};
    project.framerate = vx::FPS_29970;
    project.color_space = vx::ColorSpace::DisplayP3;
    return project;
}

void test_project_json_round_trip_is_deterministic() {
    const vx::Project original = makeProject();
    const std::string json = vx::projectToJson(original);

    const vx::Project loaded = vx::projectFromJson(json);
    const std::string saved = vx::projectToJson(loaded);

    assert(saved == json);
}

void test_project_migration_accepts_current_schema() {
    const std::string json = vx::projectToJson(makeProject());
    assert(vx::migrateProjectJsonToCurrent(json) == json);
}

void test_project_rejects_future_schema() {
    const std::string json =
        R"({"schema_version":99,"id":"00112233-4455-6677-8899-aabbccddeeff","name":"Future","created_at_ms":1,"modified_at_ms":2,"canvas":{"width":1920,"height":1080},"framerate":{"num":30,"den":1},"color_space":"rec709"})";

    try {
        static_cast<void>(vx::projectFromJson(json));
        assert(false);
    } catch (const vx::Error& e) {
        assert(e.status() == VX_ERR_UNSUPPORTED);
    }
}

}  // namespace

int main() {
    test_project_json_round_trip_is_deterministic();
    test_project_migration_accepts_current_schema();
    test_project_rejects_future_schema();
    return 0;
}
