#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

#include "resource/resource_pool.hpp"

namespace {

struct TestResourceTag;

struct TrackedResource final {
    static inline int alive = 0;

    explicit TrackedResource(std::string name) : name(std::move(name)) { ++alive; }

    TrackedResource(TrackedResource&& other) noexcept : name(std::move(other.name)) { ++alive; }

    TrackedResource& operator=(TrackedResource&&) = default;
    TrackedResource(const TrackedResource&) = delete;
    TrackedResource& operator=(const TrackedResource&) = delete;

    ~TrackedResource() { --alive; }

    std::string name;
};

using Pool = woki::gfx::detail::ResourcePool<TrackedResource, TestResourceTag>;

} // namespace

TEST_CASE("Resource pool creates and resolves typed handles") {
    TrackedResource::alive = 0;
    Pool pool;

    const auto first = pool.Emplace("first");
    const auto second = pool.Emplace("second");

    REQUIRE(pool.Size() == 2);
    REQUIRE(pool.Capacity() == 2);
    REQUIRE(pool.Contains(first));
    REQUIRE(pool.Contains(second));
    REQUIRE(pool.Get(first).name == "first");
    REQUIRE(pool.Get(second).name == "second");
    REQUIRE(TrackedResource::alive == 2);
}

TEST_CASE("Resource pool invalidates stale handles when slots are reused") {
    TrackedResource::alive = 0;
    Pool pool;

    const auto old_handle = pool.Emplace("old");
    REQUIRE(pool.Remove(old_handle));
    REQUIRE_FALSE(pool.Contains(old_handle));
    REQUIRE(pool.TryGet(old_handle) == nullptr);

    const auto new_handle = pool.Emplace("new");

    REQUIRE(new_handle.Index() == old_handle.Index());
    REQUIRE(new_handle.Generation() != old_handle.Generation());
    REQUIRE(pool.Contains(new_handle));
    REQUIRE(pool.Get(new_handle).name == "new");
    REQUIRE_FALSE(pool.Remove(old_handle));
}

TEST_CASE("Resource pool destroys values on removal and clear") {
    TrackedResource::alive = 0;
    Pool pool;

    const auto first = pool.Emplace("first");
    const auto second = pool.Emplace("second");
    REQUIRE(TrackedResource::alive == 2);

    REQUIRE(pool.Remove(first));
    REQUIRE(TrackedResource::alive == 1);

    pool.Clear();

    REQUIRE(TrackedResource::alive == 0);
    REQUIRE(pool.Empty());
    REQUIRE_FALSE(pool.Contains(second));
}

TEST_CASE("Resource pool iterates only active values") {
    Pool pool;
    const auto first = pool.Emplace("first");
    const auto removed = pool.Emplace("removed");
    const auto third = pool.Emplace("third");
    REQUIRE(pool.Remove(removed));

    std::vector<std::string> names;
    pool.Each([&](const Pool::Handle handle, const TrackedResource& resource) {
        REQUIRE(pool.Contains(handle));
        names.push_back(resource.name);
    });

    REQUIRE(names == std::vector<std::string>{"first", "third"});
    REQUIRE(pool.Contains(first));
    REQUIRE(pool.Contains(third));
}
