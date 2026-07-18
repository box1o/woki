#include <catch2/catch_test_macros.hpp>

#include <string>

#include "resource/resource_registry.hpp"

namespace {

struct RegistryResourceTag;
using Registry = woki::gfx::detail::ResourceRegistry<std::string, RegistryResourceTag>;

} // namespace

TEST_CASE("Resource registry deduplicates named assets") {
    Registry registry;
    const woki::gfx::AssetId id{"textures/checker"};

    const auto first = registry.Create(id, "Checker", woki::gfx::ResourceState::CpuReady, "first");
    const auto second =
        registry.Create(id, "Ignored", woki::gfx::ResourceState::Resident, "second");

    REQUIRE(first == second);
    REQUIRE(registry.Size() == 1);
    REQUIRE(*registry.TryGetValue(first) == "first");
    REQUIRE(registry.Find(id) == first);
}

TEST_CASE("Resource registry allows independent transient resources") {
    Registry registry;

    const auto first = registry.Create({}, "First", woki::gfx::ResourceState::Resident, "first");
    const auto second = registry.Create({}, "Second", woki::gfx::ResourceState::Resident, "second");

    REQUIRE(first != second);
    REQUIRE(registry.Size() == 2);
}

TEST_CASE("Resource replacement preserves handles and advances versions") {
    Registry registry;
    const auto handle = registry.Create(woki::gfx::AssetId{"shaders/standard"}, "Standard",
        woki::gfx::ResourceState::Resident, "version one");
    const auto initial_version = registry.TryGet(handle)->metadata.version;

    REQUIRE(registry.SetState(handle, woki::gfx::ResourceState::ReplacementPending));
    REQUIRE(registry.Replace(handle, std::string{"version two"}));

    const auto* entry = registry.TryGet(handle);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->value == "version two");
    REQUIRE(entry->metadata.version == initial_version.Next());
    REQUIRE(entry->metadata.state == woki::gfx::ResourceState::Resident);
}

TEST_CASE("Resource removal clears asset lookup and invalidates handles") {
    Registry registry;
    const woki::gfx::AssetId id{"materials/example"};
    const auto old_handle =
        registry.Create(id, "Example", woki::gfx::ResourceState::Resident, "old");

    REQUIRE(registry.Remove(old_handle));
    REQUIRE_FALSE(registry.Find(id));
    REQUIRE(registry.TryGet(old_handle) == nullptr);

    const auto new_handle =
        registry.Create(id, "Example", woki::gfx::ResourceState::Resident, "new");
    REQUIRE(new_handle != old_handle);
    REQUIRE(new_handle.Index() == old_handle.Index());
}
