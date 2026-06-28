#pragma once

#include <woki/enums.hpp>
#include <woki/window/window.hpp>

namespace woki::rhi {
class Adapter;
class BindGroup;
class BindGroupLayout;
class Buffer;
class Device;
class Instance;
class PipelineLayout;
class RenderPipeline;
class ShaderModule;
class Surface;
class Swapchain;
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
    [[nodiscard]] bool RenderFrame(f64 delta_ms);

    [[nodiscard]] bool IsReady() const noexcept { return ready_; }

private:
    [[nodiscard]] bool CreatePipelineResources();
    [[nodiscard]] bool UploadCubeResources();
    [[nodiscard]] bool UpdateUniforms(f64 delta_ms);

    scope<rhi::Instance> instance_;
    scope<rhi::Surface> surface_;
    scope<rhi::Adapter> adapter_;
    scope<rhi::Device> device_;
    scope<rhi::Swapchain> swapchain_;
    scope<rhi::ShaderModule> shader_;
    scope<rhi::BindGroupLayout> bind_group_layout_;
    scope<rhi::BindGroup> bind_group_;
    scope<rhi::Buffer> vertex_buffer_;
    scope<rhi::Buffer> index_buffer_;
    scope<rhi::Buffer> uniform_buffer_;
    scope<rhi::PipelineLayout> pipeline_layout_;
    scope<rhi::RenderPipeline> render_pipeline_;
    Window* window_{nullptr};
    u32 width_{0};
    u32 height_{0};
    rhi::TextureFormat color_format_{rhi::TextureFormat::BGRA8Unorm};
    f32 rotation_seconds_{0.0f};
    bool ready_{false};
};

} // namespace woki
