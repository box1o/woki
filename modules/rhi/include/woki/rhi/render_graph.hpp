#pragma once

#include "render_graph/bind_group_builder.hpp"
#include "render_graph/builder.hpp"
#include "render_graph/context.hpp"
#include "render_graph/internal.hpp"
#include "render_graph/resources.hpp"

#include <unordered_map>

#include <woki/core.hpp>

namespace woki::rhi {

class RenderGraphFrame final {
public:
    RenderGraphFrame(RenderGraphFrame&&) noexcept = default;
    RenderGraphFrame& operator=(RenderGraphFrame&&) noexcept = default;
    RenderGraphFrame(const RenderGraphFrame&) = delete;
    RenderGraphFrame& operator=(const RenderGraphFrame&) = delete;
    ~RenderGraphFrame();

    void Bind(PerFrameSlot slot, TextureView* view);
    void Bind(PerFrameSlot slot, Buffer* buffer);
    [[nodiscard]] Result<void> CaptureTimestamps(
        QuerySet& query_set, Buffer& resolve_buffer, Buffer& readback_buffer);
    [[nodiscard]] Result<void> Execute();

private:
    friend class RenderGraph;

    RenderGraphFrame(RenderGraph& graph, Device& device, u32 width, u32 height);

    RenderGraph* graph_{nullptr};
    Device* device_{nullptr};
    u32 width_{0};
    u32 height_{0};

    scope<CommandEncoder> encoder_{};
    std::unordered_map<u32, TextureView*> per_frame_views_{};
    QuerySet* timestamp_query_set_{nullptr};
    Buffer* timestamp_resolve_buffer_{nullptr};
    Buffer* timestamp_readback_buffer_{nullptr};
};

class RenderGraph final {
public:
    [[nodiscard]] static Result<scope<RenderGraph>> Create(
        Device& device, render_graph::detail::GraphBlueprint blueprint, u32 width, u32 height);

    [[nodiscard]] RenderGraphFrame BeginFrame(Device& device, u32 width, u32 height);
    [[nodiscard]] Result<void> RebuildForResize(u32 width, u32 height);

private:
    friend class RenderGraphBuilder;
    friend class RenderGraphFrame;

    struct RuntimeResource final {
        render_graph::detail::ResourceRecord blueprint{};
        scope<Texture> texture{};
        scope<TextureView> view{};
        scope<Buffer> buffer{};
        TextureView* per_frame_view{nullptr};
        Buffer* per_frame_buffer{nullptr};
        u32 pool_index{kInvalidGraphResource};
    };

    RenderGraph(
        Device& device, render_graph::detail::GraphBlueprint blueprint, u32 width, u32 height);

    [[nodiscard]] Result<void> AllocateRuntimeResources(u32 width, u32 height);
    void ReleaseTransientPool();
    [[nodiscard]] Result<void> AcquireTransientResource(
        RuntimeResource& runtime, u32 width, u32 height);
    [[nodiscard]] Result<void> AcquireTransientBuffer(RuntimeResource& runtime);
    [[nodiscard]] Texture* ResolveTexture(u32 resource_id);
    [[nodiscard]] TextureView* ResolveView(u32 resource_id);
    [[nodiscard]] TextureView* ResolveSampleView(u32 resource_id, SampleMode mode);
    [[nodiscard]] Buffer* ResolveBuffer(u32 resource_id);
    [[nodiscard]] Result<void> ExecuteRenderPass(u32 pass_index, CommandEncoder& encoder, u32 width,
        u32 height, const std::unordered_map<u32, TextureView*>& per_frame_views);
    [[nodiscard]] Result<void> ExecuteCopyPass(
        u32 pass_index, CommandEncoder& encoder, u32 width, u32 height);

    Device* device_{nullptr};
    render_graph::detail::GraphBlueprint blueprint_{};
    u32 width_{0};
    u32 height_{0};
    std::vector<RuntimeResource> runtime_resources_{};
    std::vector<render_graph::detail::PooledTransientTexture> transient_pool_{};
    std::vector<render_graph::detail::PooledTransientBuffer> transient_buffer_pool_{};
};

} // namespace woki::rhi
