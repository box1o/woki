#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

#include <woki/gfx/resources.hpp>

namespace {

struct TextureTag;
struct BufferTag;

using TextureHandle = woki::gfx::ResourceHandle<TextureTag>;
using BufferHandle = woki::gfx::ResourceHandle<BufferTag>;

static_assert(!std::same_as<TextureHandle, BufferHandle>);

} // namespace

TEST_CASE("Resource handles are null by default") {
    const TextureHandle handle;

    REQUIRE_FALSE(handle.Valid());
    REQUIRE_FALSE(static_cast<bool>(handle));
    REQUIRE(handle == TextureHandle::Null());
}

TEST_CASE("Resource handles preserve index generation and raw value") {
    const TextureHandle handle = TextureHandle::FromParts(42, 7);

    REQUIRE(handle.Valid());
    REQUIRE(handle.Index() == 42);
    REQUIRE(handle.Generation() == 7);
    REQUIRE(TextureHandle::FromRaw(handle.Value()) == handle);
}

TEST_CASE("Resource handles support hashing") {
    const TextureHandle first = TextureHandle::FromParts(3, 1);
    const TextureHandle same = TextureHandle::FromParts(3, 1);
    const TextureHandle newer = TextureHandle::FromParts(3, 2);

    std::unordered_set<TextureHandle> handles;
    handles.insert(first);
    handles.insert(same);
    handles.insert(newer);

    REQUIRE(handles.size() == 2);
}

TEST_CASE("Asset identifiers and resource versions are strong value types") {
    const woki::gfx::AssetId asset{"materials/standard.wmat"};
    const auto first = woki::gfx::ResourceVersion::Initial();
    const auto second = first.Next();

    REQUIRE(asset.Valid());
    REQUIRE(first.Valid());
    REQUIRE(first.Value() == 1);
    REQUIRE(second.Value() == 2);
    REQUIRE(second > first);
}

TEST_CASE("Resource states have stable diagnostic names") {
    REQUIRE(woki::gfx::ToString(woki::gfx::ResourceState::Unloaded) == "Unloaded");
    REQUIRE(woki::gfx::ToString(woki::gfx::ResourceState::Resident) == "Resident");
    REQUIRE(woki::gfx::ToString(woki::gfx::ResourceState::Retiring) == "Retiring");
}
