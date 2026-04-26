#ifndef VX_DOMAIN_TIME_H
#define VX_DOMAIN_TIME_H

#include <cstdint>
#include <compare>

#include "rational.h"

namespace vx {

/**
 * @brief Rational time representation.
 * 
 * NEVER use float seconds for timeline arithmetic. This class uses a fixed
 * timebase to ensure exact integer math for all common framerates.
 * See ADR-0005 and docs/architecture.md §6.
 */
class Time {
public:
    /**
     * @brief The default timebase used for calculations.
     * 
     * Value: 254,016,000,000. Divisible by 24, 25, 30, 50, 60, 100, 120 and 
     * corresponding NTSC fractions (24000/1001, etc.).
     */
    static constexpr int64_t DEFAULT_TIMEBASE = 254016000000LL;

    constexpr Time() = default;
    constexpr Time(int64_t ticks, int64_t timebase) : ticks_(ticks), timebase_(timebase) {}

    /**
     * @brief Factory for zero time.
     */
    [[nodiscard]] static constexpr Time zero() { return {0, DEFAULT_TIMEBASE}; }

    /**
     * @brief Create Time from floating-point seconds.
     * @warning Use sparingly as it may introduce precision loss.
     */
    [[nodiscard]] static Time fromSeconds(double seconds);

    /**
     * @brief Create Time from a frame count at a specific framerate.
     */
    [[nodiscard]] static Time fromFrames(int64_t frames, Rational fps);

    [[nodiscard]] constexpr int64_t ticks() const noexcept { return ticks_; }
    [[nodiscard]] constexpr int64_t timebase() const noexcept { return timebase_; }

    /**
     * @brief Convert to floating-point seconds.
     */
    [[nodiscard]] double toSeconds() const noexcept;

    /**
     * @brief Convert to absolute frame count for a given framerate.
     */
    [[nodiscard]] int64_t toFrames(Rational fps) const noexcept;

    /**
     * @brief Returns a copy of this time converted to a different timebase.
     */
    [[nodiscard]] Time rebased(int64_t new_timebase) const;

    Time operator+(const Time& other) const;
    Time operator-(const Time& other) const;
    bool operator==(const Time& other) const;
    std::strong_ordering operator<=>(const Time& other) const;

private:
    int64_t ticks_ = 0;           ///< Number of units (ticks).
    int64_t timebase_ = DEFAULT_TIMEBASE; ///< Units per second.
};

}  // namespace vx

#endif  // VX_DOMAIN_TIME_H
