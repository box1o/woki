#include <catch2/catch_test_macros.hpp>

#include <woki/ecs.hpp>

#include <algorithm>
#include <tuple>
#include <vector>

namespace {

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Position() noexcept = default;
    explicit constexpr Position(float value) noexcept
        : x(value), y(value) {}
    constexpr Position(float x_value, float y_value) noexcept
        : x(x_value), y(y_value) {}
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Velocity() noexcept = default;
    constexpr Velocity(float x_value, float y_value) noexcept
        : x(x_value), y(y_value) {}
};

} // namespace

TEST_CASE("Registry spawns destroys and recycles entities with new generations") {
    woki::Registry registry;

    const woki::Entity first = registry.Create();
    REQUIRE(registry.Valid(first));
    REQUIRE(registry.Size() == 1);

    REQUIRE(registry.Destroy(first));
    REQUIRE_FALSE(registry.Valid(first));
    REQUIRE(registry.Empty());

    const woki::Entity recycled = registry.Create();
    REQUIRE(registry.Valid(recycled));
    REQUIRE(recycled.Index() == first.Index());
    REQUIRE(recycled.Generation() != first.Generation());
}

TEST_CASE("Registry manages component lifetime and lookup") {
    woki::Registry registry;
    const woki::Entity entity = registry.Create();

    auto& position = registry.Emplace<Position>(entity, 1.0f, 2.0f);
    REQUIRE(position.x == 1.0f);
    REQUIRE(position.y == 2.0f);
    REQUIRE(registry.Has<Position>(entity));
    REQUIRE(registry.Count<Position>() == 1);

    auto& fetched = registry.Get<Position>(entity);
    REQUIRE(&fetched == &position);

    REQUIRE(registry.TryGet<Velocity>(entity) == nullptr);

    auto& velocity = registry.GetOrEmplace<Velocity>(entity, 3.0f, 4.0f);
    REQUIRE(velocity.x == 3.0f);
    REQUIRE(velocity.y == 4.0f);
    REQUIRE(registry.Has<Velocity>(entity));

    REQUIRE(registry.Remove<Position>(entity));
    REQUIRE_FALSE(registry.Has<Position>(entity));
    REQUIRE_FALSE(registry.Remove<Position>(entity));
}

TEST_CASE("Views iterate entities that match all requested components") {
    woki::Registry registry;
    const woki::Entity a = registry.Create();
    const woki::Entity b = registry.Create();
    const woki::Entity c = registry.Create();

    registry.Emplace<Position>(a, 1.0f, 1.0f);
    registry.Emplace<Velocity>(a, 0.5f, 1.5f);

    registry.Emplace<Position>(b, 2.0f, 2.0f);

    registry.Emplace<Position>(c, 3.0f, 3.0f);
    registry.Emplace<Velocity>(c, -1.0f, 2.0f);

    auto view = registry.View<Position, Velocity>();
    REQUIRE(view.Size() == 2);

    std::vector<woki::Entity> entities;
    for (woki::Entity entity : view) {
        entities.push_back(entity);
    }

    REQUIRE(std::find(entities.begin(), entities.end(), a) != entities.end());
    REQUIRE(std::find(entities.begin(), entities.end(), c) != entities.end());
    REQUIRE(std::find(entities.begin(), entities.end(), b) == entities.end());

    view.Each([](Position& position, Velocity& velocity) {
        position.x += velocity.x;
        position.y += velocity.y;
    });

    REQUIRE(registry.Get<Position>(a).x == 1.5f);
    REQUIRE(registry.Get<Position>(a).y == 2.5f);
    REQUIRE(registry.Get<Position>(c).x == 2.0f);
    REQUIRE(registry.Get<Position>(c).y == 5.0f);
}

TEST_CASE("Const views provide read access to matching entities") {
    woki::Registry registry;
    const woki::Entity entity = registry.Create();
    registry.Emplace<Position>(entity, 4.0f, 8.0f);
    registry.Emplace<Velocity>(entity, 1.0f, 2.0f);

    const woki::Registry& const_registry = registry;
    const auto view = const_registry.View<Position, Velocity>();

    REQUIRE_FALSE(view.Empty());

    const auto tuple = view.Get(entity);
    const auto& position = std::get<0>(tuple);
    const auto& velocity = std::get<1>(tuple);

    REQUIRE(position.x == 4.0f);
    REQUIRE(position.y == 8.0f);
    REQUIRE(velocity.x == 1.0f);
    REQUIRE(velocity.y == 2.0f);
}

TEST_CASE("Registry iterates dense live entities") {
    woki::Registry registry;
    const woki::Entity a = registry.Create();
    const woki::Entity b = registry.Create();
    const woki::Entity c = registry.Create();

    REQUIRE(registry.Destroy(b));

    std::vector<woki::Entity> entities;
    registry.EachEntity([&](woki::Entity entity) {
        entities.push_back(entity);
    });

    REQUIRE(entities.size() == 2);
    REQUIRE(std::find(entities.begin(), entities.end(), a) != entities.end());
    REQUIRE(std::find(entities.begin(), entities.end(), c) != entities.end());
    REQUIRE(std::find(entities.begin(), entities.end(), b) == entities.end());
}

TEST_CASE("Registry can query all_of any_of and clear typed storage") {
    woki::Registry registry;
    const woki::Entity entity = registry.Create();

    registry.Emplace<Position>(entity, 10.0f, 20.0f);
    registry.Emplace<Velocity>(entity, 1.0f, 2.0f);

    REQUIRE(registry.AllOf<Position, Velocity>(entity));
    REQUIRE(registry.AnyOf<Position, Velocity>(entity));

    registry.Clear<Velocity>();

    REQUIRE(registry.Has<Position>(entity));
    REQUIRE_FALSE(registry.Has<Velocity>(entity));
    REQUIRE(registry.AllOf<Position>(entity));
    REQUIRE_FALSE(registry.AllOf<Position, Velocity>(entity));
}
