#ifndef VX_DOMAIN_RATIONAL_H
#define VX_DOMAIN_RATIONAL_H

#include <cstdint>
#include <numeric>

namespace vx {

/// A rational number num/den with int64 components. Used for framerates and
/// time ratios. See ADR-0005.
struct Rational {
    int64_t num = 0;
    int64_t den = 1;

    [[nodiscard]] constexpr double toDouble() const {
        return static_cast<double>(num) / static_cast<double>(den);
    }

    [[nodiscard]] constexpr Rational reduced() const {
        if (den == 0) return {num, 1};
        const auto g = std::gcd(num < 0 ? -num : num, den < 0 ? -den : den);
        const auto sign = (den < 0) ? -1 : 1;
        return {sign * num / static_cast<int64_t>(g), sign * den / static_cast<int64_t>(g)};
    }

    constexpr bool operator==(const Rational& o) const noexcept {
        return num * o.den == o.num * den;
    }
};

// Common framerates used as defaults. Always exact, never float.
inline constexpr Rational FPS_24    {24, 1};
inline constexpr Rational FPS_25    {25, 1};
inline constexpr Rational FPS_30    {30, 1};
inline constexpr Rational FPS_50    {50, 1};
inline constexpr Rational FPS_60    {60, 1};
inline constexpr Rational FPS_23976 {24000, 1001};
inline constexpr Rational FPS_29970 {30000, 1001};
inline constexpr Rational FPS_59940 {60000, 1001};

}  // namespace vx

#endif  // VX_DOMAIN_RATIONAL_H
