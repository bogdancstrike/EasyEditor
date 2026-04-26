#ifndef VX_UTIL_UUID_H
#define VX_UTIL_UUID_H

#include <array>
#include <cstdint>
#include <string>

namespace vx {

/// 128-bit UUID (RFC 4122 v4). Used for project, sequence, clip, asset IDs.
struct Uuid {
    std::array<uint8_t, 16> bytes{};

    [[nodiscard]] static Uuid generate();
    [[nodiscard]] static Uuid fromString(const std::string& s);
    [[nodiscard]] std::string toString() const;

    bool operator==(const Uuid& other) const noexcept { return bytes == other.bytes; }
    bool operator<(const Uuid& other) const noexcept { return bytes < other.bytes; }
};

}  // namespace vx

#endif  // VX_UTIL_UUID_H
