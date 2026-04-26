#include <cassert>
#include <cstring>
#include <string>

#include "vx/public_api.h"
#include "vx/version.h"

namespace {

void test_version_and_status() {
    assert(std::strcmp(vx_version(), VX_VERSION_STRING) == 0);
    assert(std::strcmp(vx_status_message(VX_OK), "ok") == 0);
    assert(std::strcmp(vx_status_message(VX_ERR_INVALID_ARG), "invalid argument") == 0);
}

void test_project_create_serialize_destroy() {
    VxProject* project = vx_project_create("Test Project");
    assert(project != nullptr);

    vx_string_t json{nullptr, 0, nullptr};
    const vx_status_t status = vx_project_serialize_json(project, &json);
    assert(status == VX_OK);

    const std::string serialized{json.data, json.length};
    assert(serialized.find("\"schema_version\":1") != std::string::npos);
    assert(serialized.find("\"name\":\"Test Project\"") != std::string::npos);

    vx_string_free(&json);
    assert(json.data == nullptr);
    assert(json.length == 0);
    assert(json._opaque == nullptr);

    vx_project_destroy(project);
}

void test_project_load_json_round_trips() {
    VxProject* project = vx_project_create("Loaded Project");
    assert(project != nullptr);

    vx_string_t json{nullptr, 0, nullptr};
    assert(vx_project_serialize_json(project, &json) == VX_OK);

    VxProject* loaded = nullptr;
    const std::string serialized{json.data, json.length};
    const vx_status_t status = vx_project_load_json(serialized.c_str(), &loaded);
    assert(status == VX_OK);
    assert(loaded != nullptr);

    vx_project_destroy(loaded);
    vx_string_free(&json);
    vx_project_destroy(project);
}

void test_project_load_json_rejects_malformed_json() {
    VxProject* project = nullptr;
    const vx_status_t status = vx_project_load_json("{}", &project);
    assert(status == VX_ERR_INVALID_ARG);
    assert(project == nullptr);
}

}  // namespace

int main() {
    assert(vx_init() == VX_OK);
    test_version_and_status();
    test_project_create_serialize_destroy();
    test_project_load_json_round_trips();
    test_project_load_json_rejects_malformed_json();
    vx_shutdown();
    return 0;
}
