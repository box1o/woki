#pragma once

#include <compare>
#include <functional>
#include <limits>

#include <woki/core.hpp>

namespace woki::gfx {

template <typename Tag> class ResourceHandle final {
public:
    static constexpr u32 kInvalidIndex = std::numeric_limits<u32>::max();
    static constexpr u32 kInvalidGeneration = 0;

    constexpr ResourceHandle() noexcept = default;

    [[nodiscard]] static constexpr ResourceHandle FromParts(
        const u32 index, const u32 generation) noexcept {
        return ResourceHandle(index, generation);
    }

    [[nodiscard]] static constexpr ResourceHandle FromRaw(const u64 value) noexcept {
        return FromParts(static_cast<u32>(value), static_cast<u32>(value >> 32u));
    }

    [[nodiscard]] static constexpr ResourceHandle Null() noexcept { return {}; }

    [[nodiscard]] constexpr u32 Index() const noexcept { return index_; }

    [[nodiscard]] constexpr u32 Generation() const noexcept { return generation_; }

    [[nodiscard]] constexpr u64 Value() const noexcept {
        return (static_cast<u64>(generation_) << 32u) | static_cast<u64>(index_);
    }

    [[nodiscard]] constexpr bool Valid() const noexcept {
        return index_ != kInvalidIndex && generation_ != kInvalidGeneration;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return Valid(); }

    [[nodiscard]] friend constexpr bool operator==(
        ResourceHandle, ResourceHandle) noexcept = default;

    [[nodiscard]] friend constexpr auto operator<=>(
        ResourceHandle, ResourceHandle) noexcept = default;

private:
    constexpr ResourceHandle(const u32 index, const u32 generation) noexcept
        : index_(index), generation_(generation) {}

    u32 index_{kInvalidIndex};
    u32 generation_{kInvalidGeneration};
};

} // namespace woki::gfx

template <typename Tag> struct std::hash<woki::gfx::ResourceHandle<Tag>> {
    [[nodiscard]] std::size_t operator()(
        const woki::gfx::ResourceHandle<Tag> handle) const noexcept {
        return static_cast<std::size_t>(handle.Value());
    }
};
