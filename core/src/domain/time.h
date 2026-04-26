#ifndef VX_DOMAIN_TIME_H
#define VX_DOMAIN_TIME_H

#include <cstdint>

#include "rational.h"

namespace vx {

/// Rational time. NEVER use float seconds for timeline arithmetic.
/// See ADR-0005 and docs/architecture.md §6.
///
/// Default timebase = 254016000000, divisible by 24, 25, 30, 50, 60, 100, 120
/// and the corresponding NTSC fractions (24000/1001 etc). Every common framerate
/// lands on integer ticks.
class Time {
public:
    static constexpr int64_t DEFAULT_TIMEBASE = 254016000000LL;

    constexpr Time() = default;
    constexpr Time(int64_t ticks, int64_t timebase) : ticks_(ticks), timebase_(timebase) {}

    [[nodiscard]] static constexpr Time zero() { return {0, DEFAULT_TIMEBASE}; }
    [[nodiscard]] static Time fromSeconds(double seconds);
    [[nodiscard]] static Time fromFrames(int64_t frames, Rational fps);

    [[nodiscard]] constexpr int64_t ticks() const noexcept { return ticks_; }
    [[nodiscard]] constexpr int64_t timebase() const noexcept { return timebase_; }

    [[nodiscard]] double toSeconds() const noexcept;
    [[nodiscard]] int64_t toFrames(Rational fps) const noexcept;

    [[nodiscard]] Time rebased(int64_t new_timebase) const;

    Time operator+(const Time& other) const;
    Time operator-(const Time& other) const;
    bool operator==(const Time& other) const;
    bool operator<(const Time& other) const;

private:
    int64_t ticks_ = 0;
    int64_t timebase_ = DEFAULT_TIMEBASE;
};

}  // namespace vx

#endif  // VX_DOMAIN_TIME_H
