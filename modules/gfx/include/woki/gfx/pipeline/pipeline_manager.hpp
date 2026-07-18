#pragma once

#include "../resource/resource_metadata.hpp"
#include "pipeline.hpp"

#include <memory>

namespace woki::gfx {

class ShaderManager;

class PipelineManager final {
public:
    PipelineManager(rhi::Device& device, ShaderManager& shaders);
    ~PipelineManager();

    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;

    [[nodiscard]] Result<PipelineHandle> Create(const GraphicsPipelineDesc& desc);
    [[nodiscard]] Result<void> Rebuild(PipelineHandle pipeline, u64 retire_after_submission);
    [[nodiscard]] Result<void> RebuildForShader(ShaderHandle shader, u64 retire_after_submission);

    [[nodiscard]] PipelineHandle Find(AssetId asset_id) const noexcept;
    [[nodiscard]] rhi::RenderPipeline* Resolve(PipelineHandle pipeline) noexcept;
    [[nodiscard]] const rhi::RenderPipeline* Resolve(PipelineHandle pipeline) const noexcept;
    [[nodiscard]] const ResourceMetadata* Metadata(PipelineHandle pipeline) const noexcept;
    [[nodiscard]] const GraphicsPipelineDesc* Description(PipelineHandle pipeline) const noexcept;

    [[nodiscard]] bool Destroy(PipelineHandle pipeline);
    [[nodiscard]] bool Retire(PipelineHandle pipeline, u64 after_submission);
    void Collect(u64 completed_submission);

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t RetiredCount() const noexcept;
    void Clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace woki::gfx
