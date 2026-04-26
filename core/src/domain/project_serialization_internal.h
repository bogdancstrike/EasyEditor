#ifndef VX_DOMAIN_PROJECT_SERIALIZATION_INTERNAL_H
#define VX_DOMAIN_PROJECT_SERIALIZATION_INTERNAL_H

#include <chrono>
#include <cstdint>
#include "../../third_party/json/json.hpp"
#include "project.h"

using json = nlohmann::json;

namespace vx {

/**
 * @brief Serialize Resolution to JSON.
 */
inline void to_json(json& j, const Resolution& r) {
    j = json{{"width", r.width}, {"height", r.height}};
}

/**
 * @brief Deserialize Resolution from JSON.
 */
inline void from_json(const json& j, Resolution& r) {
    j.at("width").get_to(r.width);
    j.at("height").get_to(r.height);
}

/**
 * @brief Serialize Rational to JSON.
 */
inline void to_json(json& j, const Rational& r) {
    j = json{{"num", r.num}, {"den", r.den}};
}

/**
 * @brief Deserialize Rational from JSON.
 */
inline void from_json(const json& j, Rational& r) {
    j.at("num").get_to(r.num);
    j.at("den").get_to(r.den);
}

/**
 * @brief Serialize Uuid to JSON.
 */
inline void to_json(json& j, const Uuid& u) {
    j = u.toString();
}

/**
 * @brief Deserialize Uuid from JSON.
 */
inline void from_json(const json& j, Uuid& u) {
    u = Uuid::fromString(j.get<std::string>());
}

/**
 * @brief Serialize Time to JSON.
 */
inline void to_json(json& j, const Time& t) {
    j = json{{"ticks", t.ticks()}, {"timebase", t.timebase()}};
}

/**
 * @brief Deserialize Time from JSON.
 */
inline void from_json(const json& j, Time& t) {
    t = Time(j.at("ticks").get<int64_t>(), j.at("timebase").get<int64_t>());
}

/**
 * @brief Serialize ColorSpace to JSON.
 */
NLOHMANN_JSON_SERIALIZE_ENUM(ColorSpace, {
    {ColorSpace::Rec709, "Rec709"},
    {ColorSpace::Rec2020_PQ, "Rec2020_PQ"},
    {ColorSpace::Rec2020_HLG, "Rec2020_HLG"},
    {ColorSpace::DisplayP3, "DisplayP3"},
    {ColorSpace::SRGB, "SRGB"},
})

// Domain struct mappings
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MediaAsset, id, source_path, duration, resolution)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Clip, id, asset_id, timeline_start, source_in, duration)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Track, id, clips)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Sequence, id, tracks)

/**
 * @brief Convert system_clock::time_point to milliseconds since epoch.
 */
inline int64_t timePointToMs(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

/**
 * @brief Convert milliseconds since epoch to system_clock::time_point.
 */
inline std::chrono::system_clock::time_point msToTimePoint(int64_t ms) {
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
}

} // namespace vx

#endif // VX_DOMAIN_PROJECT_SERIALIZATION_INTERNAL_H
