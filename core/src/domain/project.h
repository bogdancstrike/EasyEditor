#ifndef VX_DOMAIN_PROJECT_H
#define VX_DOMAIN_PROJECT_H

#include <chrono>
#include <string>
#include <vector>

#include "rational.h"
#include "sequence.h"
#include "time.h"
#include "util/uuid.h"

namespace vx {

/**
 * @brief Represents a display or render resolution.
 */
struct Resolution {
    int width = 0;  ///< Width in pixels.
    int height = 0; ///< Height in pixels.
};

/**
 * @brief Supported color spaces for the project.
 */
enum class ColorSpace {
    Rec709,      ///< Standard Dynamic Range (BT.709).
    Rec2020_PQ,  ///< HDR with Perceptual Quantizer (BT.2020).
    Rec2020_HLG, ///< HDR with Hybrid Log-Gamma (BT.2020).
    DisplayP3,   ///< Wide Color Gamut (Apple/DCI-P3).
    SRGB         ///< Standard RGB color space.
};

/**
 * @brief Represents a source media file (video, audio, or image) in the project.
 * 
 * Media assets are stored in the project's media pool and can be referenced
 * by multiple clips across different sequences.
 */
struct MediaAsset {
    Uuid id;                ///< Unique identifier for this asset.
    std::string source_path; ///< Absolute path to the source file.
    Time duration;          ///< Total duration of the media file.
    Resolution resolution;   ///< Native resolution of the asset.
};

/**
 * @brief The project model representing an editing session (EDL).
 * 
 * Contains all metadata, media assets, and sequences that make up a video project.
 * See docs/architecture.md §6 for the full specification.
 */
struct Project {
    Uuid id;                                          ///< Unique identifier for the project.
    std::string name;                                 ///< User-defined name of the project.
    std::chrono::system_clock::time_point created_at; ///< Timestamp when the project was created.
    std::chrono::system_clock::time_point modified_at; ///< Timestamp of the last modification.

    Resolution canvas{1920, 1080};                    ///< The primary render resolution (canvas size).
    Rational framerate = FPS_30;                      ///< The project's master framerate.
    ColorSpace color_space = ColorSpace::Rec709;      ///< The project's working color space.

    std::vector<MediaAsset> media_pool;               ///< All source media available for use in the project.
    std::vector<Sequence> sequences;                  ///< The timeline(s) containing clips and effects.

    int schema_version = 1;                           ///< File format schema version for migration.
};

/// Deterministic JSON serialization for schema v1 project files.
[[nodiscard]] std::string projectToJson(const Project& p);
[[nodiscard]] Project projectFromJson(const std::string& json);

}  // namespace vx

#endif  // VX_DOMAIN_PROJECT_H
