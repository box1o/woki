#pragma once

#include "../material/material.hpp"
#include "pipeline.hpp"

#include <optional>
#include <vector>

namespace woki::gfx {

enum class RenderPassClass : u8 {
    DepthOnly = 0,
    ForwardOpaque,
    ForwardTransparent,
    Picking,
    Custom,
};

struct RenderTargetSignature final {
    std::vector<rhi::TextureFormat> color_formats{};
    std::optional<rhi::TextureFormat> depth_format{};
    u32 sample_count{1};

    [[nodiscard]] friend bool operator==(
        const RenderTargetSignature&, const RenderTargetSignature&) = default;
};

struct MaterialPipelineKey final {
    ShaderHandle shader{};
    MaterialModel model{MaterialModel::Custom};
    MaterialBlendMode blend_mode{MaterialBlendMode::Opaque};
    RenderPassClass pass{RenderPassClass::ForwardOpaque};
    RenderTargetSignature targets{};
    StringId vertex_layout{};
    bool double_sided{false};
    bool depth_write{true};

    [[nodiscard]] friend bool operator==(
        const MaterialPipelineKey&, const MaterialPipelineKey&) = default;
};

[[nodiscard]] MaterialPipelineKey MakeMaterialPipelineKey(const MaterialDesc& material,
    RenderPassClass pass, const RenderTargetSignature& targets, StringId vertex_layout);
[[nodiscard]] Result<void> Validate(const MaterialPipelineKey& key);

class MaterialPipelineResolver final {
public:
    [[nodiscard]] Result<void> Register(const MaterialPipelineKey& key, PipelineHandle pipeline);
    [[nodiscard]] Result<void> Replace(const MaterialPipelineKey& key, PipelineHandle pipeline);
    [[nodiscard]] PipelineHandle Resolve(const MaterialPipelineKey& key) const noexcept;
    [[nodiscard]] PipelineHandle Resolve(const MaterialDesc& material, RenderPassClass pass,
        const RenderTargetSignature& targets, StringId vertex_layout) const noexcept;

    [[nodiscard]] bool Remove(const MaterialPipelineKey& key);
    [[nodiscard]] std::size_t RemovePipeline(PipelineHandle pipeline);
    [[nodiscard]] bool Contains(const MaterialPipelineKey& key) const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;
    void Clear() noexcept;

private:
    struct Entry final {
        MaterialPipelineKey key{};
        PipelineHandle pipeline{};
    };

    [[nodiscard]] auto Find(const MaterialPipelineKey& key) noexcept;
    [[nodiscard]] auto Find(const MaterialPipelineKey& key) const noexcept;

    std::vector<Entry> entries_{};
};

} // namespace woki::gfx
