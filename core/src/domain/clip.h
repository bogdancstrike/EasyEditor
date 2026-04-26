#ifndef VX_DOMAIN_CLIP_H
#define VX_DOMAIN_CLIP_H

#include "time.h"
#include "util/uuid.h"

namespace vx {

/**
 * @brief Represents a single instance of a media asset on a timeline.
 * 
 * A clip references a MediaAsset but has its own timeline-specific metadata
 * such as where it starts on the timeline and which part of the source asset
 * it uses.
 */
struct Clip {
    Uuid id;             ///< Unique identifier for this clip instance.
    Uuid asset_id;       ///< ID of the MediaAsset this clip references.
    Time timeline_start; ///< Position of the clip on the sequence timeline.
    Time source_in;      ///< The start time within the source asset.
    Time duration;       ///< Duration of the clip.
};

}  // namespace vx

#endif  // VX_DOMAIN_CLIP_H
