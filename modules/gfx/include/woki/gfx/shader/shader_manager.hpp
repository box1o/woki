#pragma once

#include "../resource/resource_metadata.hpp"
#include "shader.hpp"
#include "shader_hot_reload.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include <woki/rhi.hpp>

namespace woki::gfx {

[[nodiscard]] Result<void> Validate(const ShaderDesc& desc);

class ShaderManager final {
public:
    explicit ShaderManager(rhi::Device& device, std::vector<paths::Path> include_search_paths = {});
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    [[nodiscard]] Result<ShaderHandle> Create(const ShaderDesc& desc);
    [[nodiscard]] Result<void> Reload(ShaderHandle shader, u64 retire_after_submission);
    [[nodiscard]] ShaderReloadBatch PollHotReload();

    [[nodiscard]] ShaderHandle Find(AssetId asset_id) const noexcept;
    [[nodiscard]] rhi::ShaderModule* Resolve(ShaderHandle shader, ShaderStage stage) noexcept;
    [[nodiscard]] const rhi::ShaderModule* Resolve(
        ShaderHandle shader, ShaderStage stage) const noexcept;
    [[nodiscard]] const ResourceMetadata* Metadata(ShaderHandle shader) const noexcept;
    [[nodiscard]] const ShaderDesc* Description(ShaderHandle shader) const noexcept;

    [[nodiscard]] bool Destroy(ShaderHandle shader);
    [[nodiscard]] bool Retire(ShaderHandle shader, u64 after_submission);
    void Collect(u64 completed_submission);

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t RetiredCount() const noexcept;
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
