#pragma once

#include "../resources/asset.hpp"
#include "../resources/types.hpp"

#include <string>
#include <string_view>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

enum class ShaderStage : u8 {
    Vertex = 0,
    Fragment,
    Compute,
};

enum class ShaderLanguage : u8 {
    Wgsl = 0,
    SpirV,
};

struct ShaderDefine final {
    std::string name{};
    std::string value{};
    [[nodiscard]] friend bool operator==(const ShaderDefine&, const ShaderDefine&) = default;
};

struct ShaderSource final {
    ShaderStage                 stage{ShaderStage::Vertex};
    ShaderLanguage              language{ShaderLanguage::Wgsl};
    std::string                 entry_point{"main"};
    std::string                 source{};
    std::string                 source_path{};
    std::vector<ShaderDefine>   defines{};

    [[nodiscard]] bool HasInlineSource() const noexcept { return !source.empty(); }
    [[nodiscard]] bool HasFileSource() const noexcept { return !source_path.empty(); }
};

struct ShaderDesc final {
    AssetId                     asset_id{};
    std::string                 label{};
    std::vector<ShaderSource>   sources{};
    bool                        hot_reload{true};
};

struct ShaderReloadEvent final {
    ShaderHandle    shader{};
    Version         previous_version{};
    Version         active_version{};
};

[[nodiscard]] constexpr std::string_view ToString(const ShaderStage stage) noexcept {
    switch (stage) {
    case ShaderStage::Vertex:
        return "Vertex";
    case ShaderStage::Fragment:
        return "Fragment";
    case ShaderStage::Compute:
        return "Compute";
    }

    return "Unknown";
}

} // namespace woki::gfx
