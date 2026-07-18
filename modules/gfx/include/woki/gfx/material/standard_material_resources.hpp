#pragma once

#include "../resource/gpu_resource_manager.hpp"

namespace woki::gfx {

struct StandardMaterialResources final {
    TextureHandle white{};
    TextureHandle black{};
    TextureHandle normal{};
    TextureHandle metallic_roughness{};
    SamplerHandle sampler{};

    [[nodiscard]] bool Valid() const noexcept;
};

[[nodiscard]] Result<StandardMaterialResources> CreateStandardMaterialResources(
    GpuResourceManager& resources);

} // namespace woki::gfx
