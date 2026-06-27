#include <catch2/catch_test_macros.hpp>

#include <woki/ecs.hpp>

TEST_CASE("Entity stores index generation and raw value") {
    const woki::Entity entity(42, 7);

    REQUIRE(entity.Valid());
    REQUIRE(entity.Index() == 42);
    REQUIRE(entity.Generation() == 7);

    const auto raw = entity.Value();
    const woki::Entity reconstructed = woki::Entity::FromRaw(raw);

    REQUIRE(reconstructed == entity);
}

TEST_CASE("Default entity is null") {
    const woki::Entity entity;

    REQUIRE_FALSE(entity.Valid());
    REQUIRE_FALSE(static_cast<bool>(entity));
    REQUIRE(entity == woki::Entity::Null());
}
