#include <catch2/catch_test_macros.hpp>

#include <array>

#include <woki/gfx/resources.hpp>

TEST_CASE("Frame upload arena creates aligned allocations") {
    auto arena = woki::gfx::FrameUploadArena::Create(
        {.capacity_per_frame = 64, .frames_in_flight = 2, .default_alignment = 16});
    REQUIRE(arena);
    REQUIRE(arena->BeginFrame(0, 0));
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array second{std::byte{4}, std::byte{5}};

    auto first_allocation = arena->Write(first);
    auto second_allocation = arena->Write(second);

    REQUIRE(first_allocation);
    REQUIRE(second_allocation);
    REQUIRE(first_allocation->offset == 0);
    REQUIRE(second_allocation->offset == 16);
    REQUIRE(arena->Used() == 18);
}

TEST_CASE("Frame upload arena prevents reuse of in-flight segments") {
    auto arena = woki::gfx::FrameUploadArena::Create(
        {.capacity_per_frame = 32, .frames_in_flight = 2, .default_alignment = 4});
    REQUIRE(arena);
    REQUIRE(arena->BeginFrame(0, 0));
    REQUIRE(arena->MarkSubmitted(5));

    REQUIRE_FALSE(arena->BeginFrame(2, 4));
    REQUIRE(arena->BeginFrame(2, 5));
}

TEST_CASE("Frame upload arena reports segment exhaustion without advancing") {
    auto arena = woki::gfx::FrameUploadArena::Create(
        {.capacity_per_frame = 8, .frames_in_flight = 1, .default_alignment = 4});
    REQUIRE(arena);
    REQUIRE(arena->BeginFrame(0, 0));
    const std::array data{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};
    REQUIRE(arena->Write(data));

    const std::array extra{std::byte{6}};
    REQUIRE_FALSE(arena->Write(extra));
    REQUIRE(arena->Used() == 5);
}
