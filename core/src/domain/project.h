#ifndef VX_DOMAIN_PROJECT_H
#define VX_DOMAIN_PROJECT_H

#include <chrono>
#include <string>

#include "rational.h"
#include "util/uuid.h"

namespace vx {

struct Resolution {
    int width = 0;
    int height = 0;
};

enum class ColorSpace {
    Rec709,
    Rec2020_PQ,
    Rec2020_HLG,
    DisplayP3,
    SRGB
};

/// The project model. Skeleton — fully fleshed out in Phase 0/1.
/// See docs/architecture.md §6 for the full spec; this header is the
/// minimum needed to round-trip a project file in Phase 0.
struct Project {
    Uuid id;
    std::string name;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;

    Resolution canvas{1920, 1080};
    Rational framerate = FPS_30;
    ColorSpace color_space = ColorSpace::Rec709;

    // media_pool, sequences, project_lut_stack, project_color_adjustments — Phase 1.

    int schema_version = 1;
};

/// Deterministic JSON serialization for schema v1 project files.
[[nodiscard]] std::string projectToJson(const Project& p);
[[nodiscard]] Project projectFromJson(const std::string& json);

}  // namespace vx

#endif  // VX_DOMAIN_PROJECT_H
