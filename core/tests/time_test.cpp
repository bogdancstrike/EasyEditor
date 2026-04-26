#include <cassert>

#include "domain/time.h"

namespace {

void test_ntsc_frame_round_trip() {
    const auto time = vx::Time::fromFrames(30000, vx::FPS_29970);
    assert(time.toFrames(vx::FPS_29970) == 30000);
}

void test_timebase_rebase_equality() {
    const vx::Time a{100, 1000};
    const vx::Time b{1, 10};
    assert(a == b);
}

}  // namespace

int main() {
    test_ntsc_frame_round_trip();
    test_timebase_rebase_equality();
    return 0;
}
