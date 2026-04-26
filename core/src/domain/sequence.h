#ifndef VX_DOMAIN_SEQUENCE_H
#define VX_DOMAIN_SEQUENCE_H

#include <vector>

#include "clip.h"
#include "util/uuid.h"

namespace vx {

/**
 * @brief A container for clips that are layered together.
 * 
 * Tracks represent a horizontal layer in the timeline. Clips within a track
 * are usually non-overlapping.
 */
struct Track {
    Uuid id;                ///< Unique identifier for this track.
    std::vector<Clip> clips; ///< List of clips contained in this track.
};

/**
 * @brief A collection of tracks representing a complete edit.
 * 
 * A sequence is the top-level container for an edit, consisting of multiple
 * tracks (video/audio) that are played back together.
 */
struct Sequence {
    Uuid id;                 ///< Unique identifier for this sequence.
    std::vector<Track> tracks; ///< List of tracks in this sequence.
};

}  // namespace vx

#endif  // VX_DOMAIN_SEQUENCE_H
