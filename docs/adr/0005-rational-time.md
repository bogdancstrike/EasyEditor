# 0005 — Rational time, never float

- **Status:** Accepted
- **Date:** 2026-04-26
- **Tags:** `core`, `domain`

## Context

Common framerates are not float-friendly:

- 23.976 fps = 24000 / 1001
- 29.97 fps = 30000 / 1001
- 59.94 fps = 60000 / 1001

A timeline using `double` for time accumulates drift across thousands of frames. The drift causes audio-video desync, missed frame boundaries during export, and "off by one frame" bugs that are nearly impossible to track down because they only show up after long sequences.

Every professional video toolkit (FFmpeg's `AVRational`, AVFoundation's `CMTime`, Adobe's internal time types) uses rational time for the same reason.

## Decision

The engine uses a `vx::Time` type backed by `int64_t ticks / int64_t timebase`. The default timebase is **254 016 000 000** — divisible by 24, 25, 30, 50, 60, 100, 120 and the corresponding NTSC fractions, so all common framerates land on integer ticks.

```cpp
struct Time {
    int64_t ticks;
    int64_t timebase;
    static Time fromSeconds(double s);
    static Time fromFrames(int64_t n, Rational fps);
    int64_t toFrames(Rational fps) const;
    double toSeconds() const;
};
```

Float conversion exists at the **edges only** — UI display, audio engine APIs, debug logs.

## Alternatives considered

- **`double` seconds.** Simple, broken at scale. Rejected.
- **`int64_t` microseconds.** Works for 30 fps and round numbers; doesn't represent 30000/1001 exactly. Rejected.
- **Float frames.** Same drift problem with a different unit. Rejected.

## Consequences

- **Positive:** Frame-accurate timeline arithmetic. Audio-video sync without drift. Exact equality comparisons work.
- **Negative:** Slightly more verbose than `double`. Operators (`+`, `-`, `*`) need careful implementation handling timebase normalization.
- **Negative:** Anyone touching the engine must learn the pattern. Documented in `docs/architecture.md` §6 and in `vx/time.h` itself.

## References

- `docs/architecture.md` §6
- FFmpeg's `AVRational` and `av_rescale_q`
- Apple's `CMTime` documentation
