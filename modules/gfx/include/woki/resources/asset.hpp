#pragma once

#include <functional>
#include <string_view>

#include <woki/core.hpp>

namespace woki::gfx {

class AssetId final {
public:
    constexpr AssetId() noexcept = default;

    explicit constexpr AssetId(const StringId value) noexcept : value_(value) {}
    explicit constexpr AssetId(const std::string_view value) noexcept : value_(value) {}

    [[nodiscard]] constexpr StringId Value() const noexcept { return value_; }
    [[nodiscard]] constexpr bool Valid() const noexcept { return !value_.Empty(); }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return Valid(); }
    [[nodiscard]] friend constexpr bool operator==(AssetId, AssetId) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(AssetId, AssetId) noexcept = default;

private:
    StringId value_{};
};

class Version final {
public:
    static constexpr u32 kInvalidValue = 0;
    static constexpr u32 kInitialValue = 1;

    constexpr Version() noexcept = default;

    explicit constexpr Version(const u32 value) noexcept : value_(value) {}

    [[nodiscard]] static constexpr Version Initial() noexcept {
        return Version(kInitialValue);
    }

    [[nodiscard]] constexpr u32 Value() const noexcept { return value_; }
    [[nodiscard]] constexpr bool Valid() const noexcept { return value_ != kInvalidValue; }
    [[nodiscard]] constexpr Version Next() const noexcept {
        const u32 next = value_ + 1u;
        return Version(next == kInvalidValue ? kInitialValue : next);
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return Valid(); }
    [[nodiscard]] friend constexpr bool operator==( Version, Version) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>( Version, Version) noexcept = default;

private:
    u32 value_{kInvalidValue};
};

} // namespace woki::gfx

template <> struct std::hash<woki::gfx::AssetId> {
    [[nodiscard]] std::size_t operator()(const woki::gfx::AssetId id) const noexcept {
        return std::hash<woki::StringId>{}(id.Value());
    }
};
