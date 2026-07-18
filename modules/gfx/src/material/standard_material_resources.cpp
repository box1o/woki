#include <woki/gfx/material/standard_material_resources.hpp>

#include <array>

namespace woki::gfx {
namespace {

[[nodiscard]] Result<TextureHandle> CreatePixel(GpuResourceManager& resources,
    const std::string_view asset, const std::string_view label,
    const std::array<std::byte, 4> pixel) {
    TextureResourceDesc desc{};
    desc.asset_id = AssetId{asset};
    desc.gpu.size = {.width = 1, .height = 1, .depth_or_array_layers = 1};
    desc.gpu.format = rhi::TextureFormat::RGBA8Unorm;
    desc.gpu.usage = rhi::TextureUsage::TextureBinding | rhi::TextureUsage::CopyDst;
    desc.gpu.label = std::string(label);
    desc.initial_data = {{
        .data = std::vector<std::byte>(pixel.begin(), pixel.end()),
        .bytes_per_row = 4,
        .rows_per_image = 1,
    }};
    return resources.CreateTexture(desc);
}

} // namespace

bool StandardMaterialResources::Valid() const noexcept {
    return white && black && normal && metallic_roughness && sampler && shadow_sampler;
}

Result<StandardMaterialResources> CreateStandardMaterialResources(GpuResourceManager& resources) {
    auto white = CreatePixel(resources, "woki/textures/white", "Woki white texture",
        {std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}});
    if (!white) {
        return Err(white.error());
    }
    auto black = CreatePixel(resources, "woki/textures/black", "Woki black texture",
        {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}});
    if (!black) {
        return Err(black.error());
    }
    auto normal = CreatePixel(resources, "woki/textures/normal", "Woki flat normal texture",
        {std::byte{128}, std::byte{128}, std::byte{255}, std::byte{255}});
    if (!normal) {
        return Err(normal.error());
    }
    auto metallic_roughness = CreatePixel(resources, "woki/textures/metallic_roughness",
        "Woki metallic roughness texture",
        {std::byte{255}, std::byte{255}, std::byte{0}, std::byte{255}});
    if (!metallic_roughness) {
        return Err(metallic_roughness.error());
    }
    auto sampler = resources.CreateSampler({
        .asset_id = AssetId{"woki/samplers/linear_repeat"},
        .gpu = {.address_mode_u = rhi::AddressMode::Repeat,
            .address_mode_v = rhi::AddressMode::Repeat,
            .address_mode_w = rhi::AddressMode::Repeat,
            .mag_filter = rhi::FilterMode::Linear,
            .min_filter = rhi::FilterMode::Linear,
            .mipmap_filter = rhi::MipmapFilterMode::Linear,
            .label = "Woki linear repeat sampler"},
    });
    if (!sampler) {
        return Err(sampler.error());
    }
    auto shadow_sampler = resources.CreateSampler({
        .asset_id = AssetId{"woki/samplers/shadow_comparison"},
        .gpu = {.address_mode_u = rhi::AddressMode::ClampToEdge,
            .address_mode_v = rhi::AddressMode::ClampToEdge,
            .address_mode_w = rhi::AddressMode::ClampToEdge,
            .mag_filter = rhi::FilterMode::Linear,
            .min_filter = rhi::FilterMode::Linear,
            .mipmap_filter = rhi::MipmapFilterMode::Nearest,
            .compare = rhi::CompareFunction::LessEqual,
            .label = "Woki shadow comparison sampler"},
    });
    if (!shadow_sampler) {
        return Err(shadow_sampler.error());
    }
    return Ok(StandardMaterialResources{.white = *white,
        .black = *black,
        .normal = *normal,
        .metallic_roughness = *metallic_roughness,
        .sampler = *sampler,
        .shadow_sampler = *shadow_sampler});
}

} // namespace woki::gfx
