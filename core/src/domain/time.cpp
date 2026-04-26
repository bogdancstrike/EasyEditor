#include "time.h"

#include <cmath>

#include "util/error.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

namespace vx {

Time Time::fromSeconds(double seconds) {
    return Time{static_cast<int64_t>(std::llround(seconds * DEFAULT_TIMEBASE)), DEFAULT_TIMEBASE};
}

Time Time::fromFrames(int64_t frames, Rational fps) {
    if (fps.num == 0) throw Error(VX_ERR_INVALID_ARG, "fps numerator cannot be 0");
    // ticks = frames * timebase * fps.den / fps.num
    const __int128 prod = static_cast<__int128>(frames) * DEFAULT_TIMEBASE * fps.den;
    return Time{static_cast<int64_t>(prod / fps.num), DEFAULT_TIMEBASE};
}

double Time::toSeconds() const noexcept {
    return static_cast<double>(ticks_) / static_cast<double>(timebase_);
}

int64_t Time::toFrames(Rational fps) const noexcept {
    // frames = ticks * fps.num / (timebase * fps.den)
    const __int128 num = static_cast<__int128>(ticks_) * fps.num;
    const __int128 den = static_cast<__int128>(timebase_) * fps.den;
    return static_cast<int64_t>(num / den);
}

Time Time::rebased(int64_t new_timebase) const {
    if (new_timebase == timebase_) return *this;
    const __int128 t = static_cast<__int128>(ticks_) * new_timebase / timebase_;
    return Time{static_cast<int64_t>(t), new_timebase};
}

Time Time::operator+(const Time& other) const {
    if (timebase_ == other.timebase_) return Time{ticks_ + other.ticks_, timebase_};
    return *this + other.rebased(timebase_);
}

Time Time::operator-(const Time& other) const {
    if (timebase_ == other.timebase_) return Time{ticks_ - other.ticks_, timebase_};
    return *this - other.rebased(timebase_);
}

bool Time::operator==(const Time& other) const {
    if (timebase_ == other.timebase_) return ticks_ == other.ticks_;
    return rebased(other.timebase_).ticks_ == other.ticks_;
}

bool Time::operator<(const Time& other) const {
    if (timebase_ == other.timebase_) return ticks_ < other.ticks_;
    return rebased(other.timebase_).ticks_ < other.ticks_;
}

}  // namespace vx

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
