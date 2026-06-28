#pragma once

#include <woki/enums.hpp>
#include <woki/events/events.hpp>
#include <woki/rhi/render_graph/resources.hpp>
#include <woki/window/window.hpp>

namespace woki::rhi {
class Adapter;
class BindGroup;
class BindGroupLayout;
class Buffer;
class Device;
class Instance;
class PipelineLayout;
class RenderGraph;
class RenderPipeline;
class Sampler;
class ShaderModule;
class Surface;
class Swapchain;
class Texture;
class TextureView;
}

namespace woki {

class RhiRenderer {
public:
    RhiRenderer();
    ~RhiRenderer();

    RhiRenderer(const RhiRenderer&) = delete;
    RhiRenderer& operator=(const RhiRenderer&) = delete;

    [[nodiscard]] bool Initialize(Window& window);
    void Shutdown() noexcept;
    void Resize(u32 width, u32 height);
    void HandleEvent(events::Event& event);
    [[nodiscard]] bool RenderFrame(f64 delta_ms);

    [[nodiscard]] bool IsReady() const noexcept { return ready_; }

private:
    [[nodiscard]] bool CreateDeferredPipelines();
    [[nodiscard]] bool BuildRenderGraph();
    [[nodiscard]] bool UploadCubeResources();
    [[nodiscard]] bool UpdateUniforms(f64 delta_ms);

    scope<rhi::Instance> instance_;
    scope<rhi::Surface> surface_;
    scope<rhi::Adapter> adapter_;
    scope<rhi::Device> device_;
    scope<rhi::Swapchain> swapchain_;
    scope<rhi::ShaderModule> gbuffer_shader_;
    scope<rhi::ShaderModule> lighting_shader_;
    scope<rhi::ShaderModule> present_shader_;
    scope<rhi::ShaderModule> texture_debug_shader_;
    scope<rhi::BindGroupLayout> gbuffer_bind_group_layout_;
    scope<rhi::BindGroupLayout> lighting_bind_group_layout_;
    scope<rhi::BindGroupLayout> present_bind_group_layout_;
    scope<rhi::BindGroup> gbuffer_bind_group_;
    scope<rhi::Buffer> vertex_buffer_;
    scope<rhi::Buffer> index_buffer_;
    scope<rhi::Buffer> uniform_buffer_;
    scope<rhi::PipelineLayout> gbuffer_pipeline_layout_;
    scope<rhi::PipelineLayout> lighting_pipeline_layout_;
    scope<rhi::PipelineLayout> present_pipeline_layout_;
    scope<rhi::RenderPipeline> gbuffer_pipeline_;
    scope<rhi::RenderPipeline> lighting_pipeline_;
    scope<rhi::RenderPipeline> present_pipeline_;
    scope<rhi::RenderPipeline> texture_debug_pipeline_;
    scope<rhi::Sampler> linear_sampler_;
    scope<rhi::Texture> history_texture_;
    scope<rhi::RenderGraph> render_graph_;
    rhi::PerFrameSlot backbuffer_{};
    rhi::Resource gbuffer_albedo_{};
    rhi::Resource gbuffer_normal_{};
    rhi::Resource gbuffer_material_{};
    rhi::Resource gbuffer_depth_{};
    rhi::Resource hdr_color_{};
    rhi::Resource history_{};
    Window* window_{nullptr};
    u32 width_{0};
    u32 height_{0};
    rhi::TextureFormat color_format_{rhi::TextureFormat::BGRA8Unorm};
    f32 cube_yaw_{0.65f};
    f32 cube_pitch_{0.45f};
    bool rotating_with_mouse_{false};
    bool show_texture_debug_{false};
    bool ready_{false};
};

} // namespace woki
