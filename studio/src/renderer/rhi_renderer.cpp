#include "rhi_renderer.hpp"

#include <woki/rhi.hpp>

#include <array>
namespace woki {

namespace {

constexpr const char* kTriangleWgsl = R"(
struct VertexOutput {
    @builtin(position) position: vec4f,
};

@vertex
fn vs_main(@location(0) pos: vec2f) -> VertexOutput {
    var output: VertexOutput;
    output.position = vec4f(pos, 0.0, 1.0);
    return output;
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.92, 0.35, 0.18, 1.0);
}
)";

constexpr std::array<f32, 6> kTriangleVertices = {
    0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
};

} // namespace

bool RhiRenderer::CreatePipelineResources() {
    rhi::ShaderModuleDesc shader_desc{};
    shader_desc.label = "StudioTriangleShader";
    shader_desc.code = kTriangleWgsl;
    auto shader = device_->CreateShaderModule(shader_desc);
    if (!shader) {
        slog::Error("RHI shader creation failed: {}", shader.error().Message());
        return false;
    }
    shader_ = std::move(*shader);

    rhi::PipelineLayoutDesc layout_desc{};
    layout_desc.label = "StudioPipelineLayout";
    auto pipeline_layout = device_->CreatePipelineLayout(layout_desc);
    if (!pipeline_layout) {
        slog::Error("RHI pipeline layout creation failed: {}", pipeline_layout.error().Message());
        return false;
    }
    pipeline_layout_ = std::move(*pipeline_layout);

    constexpr u64 kVertexBufferSize = sizeof(kTriangleVertices);
    rhi::BufferDesc buffer_desc{};
    buffer_desc.label = "StudioTriangleVertices";
    buffer_desc.size = kVertexBufferSize;
    buffer_desc.usage = static_cast<rhi::BufferUsage>(
        static_cast<u64>(rhi::BufferUsage::Vertex) | static_cast<u64>(rhi::BufferUsage::CopyDst));
    auto vertex_buffer = device_->CreateBuffer(buffer_desc);
    if (!vertex_buffer) {
        slog::Error("RHI vertex buffer creation failed: {}", vertex_buffer.error().Message());
        return false;
    }
    vertex_buffer_ = std::move(*vertex_buffer);

    if (auto write = device_->GetQueue().WriteBuffer(
            *vertex_buffer_, 0, kTriangleVertices.data(), kVertexBufferSize);
        !write) {
        slog::Error("RHI vertex buffer upload failed: {}", write.error().Message());
        return false;
    }

    const rhi::VertexAttributeDesc vertex_attribute{
        .format = rhi::VertexFormat::Float32x2,
        .offset = 0,
        .shader_location = 0,
    };
    const rhi::VertexBufferLayoutDesc vertex_buffer_layout{
        .array_stride = sizeof(f32) * 2,
        .attributes = std::span<const rhi::VertexAttributeDesc>(&vertex_attribute, 1),
    };
    const rhi::VertexStateDesc vertex_state{
        .module = shader_.get(),
        .entry_point = "vs_main",
        .buffers = std::span<const rhi::VertexBufferLayoutDesc>(&vertex_buffer_layout, 1),
    };

    const rhi::ColorTargetStateDesc color_target{
        .format = rhi::TextureFormat::BGRA8Unorm,
    };
    const rhi::FragmentStateDesc fragment_state{
        .module = shader_.get(),
        .entry_point = "fs_main",
        .targets = std::span<const rhi::ColorTargetStateDesc>(&color_target, 1),
    };

    const rhi::PrimitiveStateDesc primitive_state{};

    const rhi::DepthStencilStateDesc depth_stencil_state{};

    rhi::RenderPipelineDescTyped pipeline_desc{};
    pipeline_desc.label = "StudioTrianglePipeline";
    pipeline_desc.layout = pipeline_layout_.get();
    pipeline_desc.vertex = &vertex_state;
    pipeline_desc.primitive = &primitive_state;
    pipeline_desc.depth_stencil = &depth_stencil_state;
    pipeline_desc.fragment = &fragment_state;

    auto render_pipeline = device_->CreateRenderPipeline(pipeline_desc);
    if (!render_pipeline) {
        slog::Error("RHI render pipeline creation failed: {}", render_pipeline.error().Message());
        return false;
    }
    render_pipeline_ = std::move(*render_pipeline);
    return true;
}

RhiRenderer::RhiRenderer() = default;

RhiRenderer::~RhiRenderer() {
    Shutdown();
}

bool RhiRenderer::Initialize(Window& window) {
    Shutdown();

    window_ = &window;
    width_ = window.GetWidth();
    height_ = window.GetHeight();

    auto instance = rhi::Instance::Create({});
    if (!instance) {
        slog::Error("RHI init failed: {}", instance.error().Message());
        return false;
    }
    instance_ = std::move(*instance);

    auto surface = instance_->CreateSurface(window);
    if (!surface) {
        slog::Error("RHI surface creation failed: {}", surface.error().Message());
        Shutdown();
        return false;
    }
    surface_ = std::move(*surface);

    rhi::RequestAdapterDesc adapter_desc{};
    adapter_desc.compatible_surface = surface_.get();
    auto adapter = instance_->RequestAdapter(adapter_desc);
    if (!adapter) {
        slog::Error("RHI adapter request failed: {}", adapter.error().Message());
        Shutdown();
        return false;
    }
    adapter_ = std::move(*adapter);

    auto device = adapter_->CreateDevice({});
    if (!device) {
        slog::Error("RHI device creation failed: {}", device.error().Message());
        Shutdown();
        return false;
    }
    device_ = std::move(*device);

    auto swapchain = rhi::Swapchain::Builder(*device_, *surface_)
                         .SizeSource(window_)
                         .Label("StudioSwapchain")
                         .Build();
    if (!swapchain) {
        slog::Error("RHI swapchain creation failed: {}", swapchain.error().Message());
        Shutdown();
        return false;
    }
    swapchain_ = std::move(*swapchain);

    if (!CreatePipelineResources()) {
        Shutdown();
        return false;
    }

    ready_ = true;
    slog::Info("RHI renderer initialized ({}x{})", width_, height_);
    return true;
}

void RhiRenderer::Shutdown() noexcept {
    render_pipeline_.reset();
    vertex_buffer_.reset();
    pipeline_layout_.reset();
    shader_.reset();
    swapchain_.reset();
    device_.reset();
    adapter_.reset();
    surface_.reset();
    instance_.reset();
    window_ = nullptr;
    width_ = 0;
    height_ = 0;
    ready_ = false;
}

void RhiRenderer::Resize(const u32 width, const u32 height) {
    if (!ready_ || swapchain_ == nullptr) {
        return;
    }

    if (width == 0 || height == 0 || (width == width_ && height == height_)) {
        return;
    }

    width_ = width;
    height_ = height;
    swapchain_->Resize(width, height);
}

bool RhiRenderer::RenderFrame() {
    if (!ready_ || device_ == nullptr || swapchain_ == nullptr || instance_ == nullptr) {
        return false;
    }

    if (window_ != nullptr) {
        const u32 window_width = window_->GetWidth();
        const u32 window_height = window_->GetHeight();
        if (window_width != width_ || window_height != height_) {
            Resize(window_width, window_height);
        }
    }

    auto frame = swapchain_->AcquireNextFrame();
    if (!frame) {
        slog::Warn("Failed to acquire swapchain frame: {}", frame.error().Message());
        instance_->ProcessEvents();
        return false;
    }

    auto encoder_result = device_->CreateCommandEncoder({});
    if (!encoder_result) {
        slog::Warn("Failed to create command encoder: {}", encoder_result.error().Message());
        return false;
    }
    auto encoder = std::move(*encoder_result);

    rhi::RenderPassColorAttachmentDesc color_attachment{};
    color_attachment.view = &frame->ColorView();

    rhi::RenderPassDescTyped pass_desc{};
    pass_desc.label = "StudioRenderPass";
    pass_desc.color_attachments = std::span<const rhi::RenderPassColorAttachmentDesc>(&color_attachment, 1);

    rhi::RenderPassDepthStencilAttachmentDesc depth_attachment{};
    if (rhi::TextureView* depth_view = frame->DepthView(); depth_view != nullptr) {
        depth_attachment.view = depth_view;
        pass_desc.depth_stencil_attachment = &depth_attachment;
    }

    auto pass_result = encoder->BeginRenderPass(pass_desc);
    if (!pass_result) {
        slog::Warn("Failed to begin render pass: {}", pass_result.error().Message());
        return false;
    }
    auto pass = std::move(*pass_result);
    pass->SetPipeline(*render_pipeline_);
    pass->SetVertexBuffer(0, vertex_buffer_.get());
    pass->Draw(3);
    pass->End();

    auto command_buffer_result = encoder->Finish({});
    if (!command_buffer_result) {
        slog::Warn("Failed to finish command encoder: {}", command_buffer_result.error().Message());
        return false;
    }
    auto command_buffer = std::move(*command_buffer_result);

    rhi::CommandBuffer* buffers[] = {command_buffer.get()};
    if (auto submit = device_->GetQueue().Submit(buffers); !submit) {
        slog::Warn("Queue submit failed: {}", submit.error().Message());
        return false;
    }

    if (auto present = swapchain_->Present(); !present) {
        slog::Warn("Swapchain present failed: {}", present.error().Message());
        return false;
    }

    device_->Tick();
    instance_->ProcessEvents();
    return true;
}

} // namespace woki
