#include <catch2/catch_test_macros.hpp>

#include <woki/core.hpp>

TEST_CASE("HashString is stable for identical values") {
    const auto a = woki::HashString("hello");
    const auto b = woki::HashString("hello");
    const auto c = woki::HashString("world");

    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST_CASE("HashCombine updates seed") {
    std::size_t seed = 0;
    woki::HashCombine(seed, 42);
    REQUIRE(seed != 0);
}

TEST_CASE("StringId hashes strings consistently") {
    const woki::StringId a("player");
    const woki::StringId b("player");
    const woki::StringId c("enemy");

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE_FALSE(a.Empty());
}
