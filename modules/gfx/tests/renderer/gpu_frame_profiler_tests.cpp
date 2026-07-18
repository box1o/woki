#include <catch2/catch_test_macros.hpp>

#include "renderer/gpu_frame_profiler.hpp"

TEST_CASE("GPU frame duration resolves monotonic timestamps") {
    const auto duration = woki::gfx::detail::ResolveGpuDuration(1'000, 4'250);

    REQUIRE(duration);
    REQUIRE(*duration == 3'250);
}

TEST_CASE("GPU frame duration rejects reversed timestamps") {
    REQUIRE_FALSE(woki::gfx::detail::ResolveGpuDuration(5'000, 4'999));
}
