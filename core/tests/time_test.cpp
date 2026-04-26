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

void test_comparisons() {
    const vx::Time t1{10, 100};
    const vx::Time t2{20, 100};
    const vx::Time t3{1, 10}; // same as t1
    
    assert(t1 < t2);
    assert(t1 <= t2);
    assert(t2 > t1);
    assert(t2 >= t1);
    assert(t1 == t3);
    assert(t1 <= t3);
    assert(t1 >= t3);
}

}  // namespace

int main() {
    test_ntsc_frame_round_trip();
    test_timebase_rebase_equality();
    test_comparisons();
    return 0;
}
