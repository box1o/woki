#include "rhi_renderer.hpp"

#include <woki/math/math.hpp>
#include <woki/rhi.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace woki {

namespace {

struct CubeVertex final {
    math::vec3f position;
    math::vec3f color;
};

struct alignas(16) CubeUniforms final {
    math::mat4f mvp;
};

constexpr const char* kCubeWgsl = R"(
struct Uniforms {
    mvp: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@vertex
fn vs_main(@location(0) position: vec3f, @location(1) color: vec3f) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.mvp * vec4f(position, 1.0);
    output.color = color;
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(input.color, 1.0);
}
)";

constexpr std::array<CubeVertex, 24> kCubeVertices = {
    // Front
    CubeVertex{{-1.0f, -1.0f,  1.0f}, {0.93f, 0.25f, 0.21f}},
    CubeVertex{{ 1.0f, -1.0f,  1.0f}, {0.93f, 0.25f, 0.21f}},
    CubeVertex{{ 1.0f,  1.0f,  1.0f}, {0.93f, 0.25f, 0.21f}},
    CubeVertex{{-1.0f,  1.0f,  1.0f}, {0.93f, 0.25f, 0.21f}},
    // Back
    CubeVertex{{ 1.0f, -1.0f, -1.0f}, {0.16f, 0.55f, 0.96f}},
    CubeVertex{{-1.0f, -1.0f, -1.0f}, {0.16f, 0.55f, 0.96f}},
    CubeVertex{{-1.0f,  1.0f, -1.0f}, {0.16f, 0.55f, 0.96f}},
    CubeVertex{{ 1.0f,  1.0f, -1.0f}, {0.16f, 0.55f, 0.96f}},
    // Left
    CubeVertex{{-1.0f, -1.0f, -1.0f}, {0.26f, 0.78f, 0.39f}},
    CubeVertex{{-1.0f, -1.0f,  1.0f}, {0.26f, 0.78f, 0.39f}},
    CubeVertex{{-1.0f,  1.0f,  1.0f}, {0.26f, 0.78f, 0.39f}},
    CubeVertex{{-1.0f,  1.0f, -1.0f}, {0.26f, 0.78f, 0.39f}},
    // Right
    CubeVertex{{ 1.0f, -1.0f,  1.0f}, {0.96f, 0.70f, 0.18f}},
    CubeVertex{{ 1.0f, -1.0f, -1.0f}, {0.96f, 0.70f, 0.18f}},
    CubeVertex{{ 1.0f,  1.0f, -1.0f}, {0.96f, 0.70f, 0.18f}},
    CubeVertex{{ 1.0f,  1.0f,  1.0f}, {0.96f, 0.70f, 0.18f}},
    // Top
    CubeVertex{{-1.0f,  1.0f,  1.0f}, {0.65f, 0.38f, 0.92f}},
    CubeVertex{{ 1.0f,  1.0f,  1.0f}, {0.65f, 0.38f, 0.92f}},
    CubeVertex{{ 1.0f,  1.0f, -1.0f}, {0.65f, 0.38f, 0.92f}},
    CubeVertex{{-1.0f,  1.0f, -1.0f}, {0.65f, 0.38f, 0.92f}},
    // Bottom
    CubeVertex{{-1.0f, -1.0f, -1.0f}, {0.07f, 0.72f, 0.73f}},
    CubeVertex{{ 1.0f, -1.0f, -1.0f}, {0.07f, 0.72f, 0.73f}},
    CubeVertex{{ 1.0f, -1.0f,  1.0f}, {0.07f, 0.72f, 0.73f}},
    CubeVertex{{-1.0f, -1.0f,  1.0f}, {0.07f, 0.72f, 0.73f}},
};

constexpr std::array<u16, 36> kCubeIndices = {
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};

[[nodiscard]] math::mat4f PerspectiveWebGpu(const f32 fovy, const f32 aspect, const f32 z_near, const f32 z_far) {
    const f32 tan_half = std::tan(fovy * 0.5f);
    const f32 depth = z_near - z_far;

    return math::mat4f(math::layout::rowm,
        1.0f / (aspect * tan_half), 0.0f,             0.0f,                    0.0f,
        0.0f,                         1.0f / tan_half, 0.0f,                    0.0f,
        0.0f,                         0.0f,            z_far / depth,           (z_near * z_far) / depth,
        0.0f,                         0.0f,           -1.0f,                    0.0f);
}

} // namespace

bool RhiRenderer::UploadCubeResources() {
    const auto upload_buffer = [this](const rhi::Buffer& buffer, const void* data, const u64 size, const char* label) {
        if (auto write = device_->GetQueue().WriteBuffer(buffer, 0, data, size); !write) {
            slog::Error("RHI {} upload failed: {}", label, write.error().Message());
            return false;
        }
        return true;
    };

    rhi::BufferDesc vertex_buffer_desc{};
    vertex_buffer_desc.label = "StudioCubeVertices";
    vertex_buffer_desc.size = sizeof(kCubeVertices);
    vertex_buffer_desc.usage = rhi::BufferUsage::Vertex | rhi::BufferUsage::CopyDst;
    auto vertex_buffer = device_->CreateBuffer(vertex_buffer_desc);
    if (!vertex_buffer) {
        slog::Error("RHI vertex buffer creation failed: {}", vertex_buffer.error().Message());
        return false;
    }
    vertex_buffer_ = std::move(*vertex_buffer);

    if (!upload_buffer(*vertex_buffer_, kCubeVertices.data(), sizeof(kCubeVertices), "vertex buffer")) {
        return false;
    }

    rhi::BufferDesc index_buffer_desc{};
    index_buffer_desc.label = "StudioCubeIndices";
    index_buffer_desc.size = sizeof(kCubeIndices);
    index_buffer_desc.usage = rhi::BufferUsage::Index | rhi::BufferUsage::CopyDst;
    auto index_buffer = device_->CreateBuffer(index_buffer_desc);
    if (!index_buffer) {
        slog::Error("RHI index buffer creation failed: {}", index_buffer.error().Message());
        return false;
    }
    index_buffer_ = std::move(*index_buffer);

    if (!upload_buffer(*index_buffer_, kCubeIndices.data(), sizeof(kCubeIndices), "index buffer")) {
        return false;
    }

    rhi::BufferDesc uniform_buffer_desc{};
    uniform_buffer_desc.label = "StudioCubeUniforms";
    uniform_buffer_desc.size = sizeof(CubeUniforms);
    uniform_buffer_desc.usage = rhi::BufferUsage::Uniform | rhi::BufferUsage::CopyDst;
    auto uniform_buffer = device_->CreateBuffer(uniform_buffer_desc);
    if (!uniform_buffer) {
        slog::Error("RHI uniform buffer creation failed: {}", uniform_buffer.error().Message());
        return false;
    }
    uniform_buffer_ = std::move(*uniform_buffer);

    return UpdateUniforms(0.0);
}

bool RhiRenderer::CreatePipelineResources() {
    rhi::ShaderModuleDesc shader_desc{};
    shader_desc.label = "StudioCubeShader";
    shader_desc.code = kCubeWgsl;
    auto shader = device_->CreateShaderModule(shader_desc);
    if (!shader) {
        slog::Error("RHI shader creation failed: {}", shader.error().Message());
        return false;
    }
    shader_ = std::move(*shader);

    if (!UploadCubeResources()) {
        return false;
    }

    const rhi::BindGroupLayoutEntryDesc uniform_layout_entry{
        .binding = 0,
        .visibility = static_cast<u32>(rhi::ShaderStage::Vertex),
        .buffer = rhi::BufferBindingLayoutDesc{
            .type = rhi::BufferBindingType::Uniform,
            .min_binding_size = sizeof(CubeUniforms),
        },
    };

    rhi::BindGroupLayoutDesc bind_group_layout_desc{};
    bind_group_layout_desc.label = "StudioCubeBindGroupLayout";
    bind_group_layout_desc.entries = std::span<const rhi::BindGroupLayoutEntryDesc>(&uniform_layout_entry, 1);
    auto bind_group_layout = device_->CreateBindGroupLayout(bind_group_layout_desc);
    if (!bind_group_layout) {
        slog::Error("RHI bind group layout creation failed: {}", bind_group_layout.error().Message());
        return false;
    }
    bind_group_layout_ = std::move(*bind_group_layout);

    const rhi::BindGroupEntryDesc uniform_binding{
        .binding = 0,
        .buffer = uniform_buffer_.get(),
        .offset = 0,
        .size = sizeof(CubeUniforms),
    };

    rhi::BindGroupDesc bind_group_desc{};
    bind_group_desc.label = "StudioCubeBindGroup";
    bind_group_desc.layout = bind_group_layout_.get();
    bind_group_desc.entries = std::span<const rhi::BindGroupEntryDesc>(&uniform_binding, 1);
    auto bind_group = device_->CreateBindGroup(bind_group_desc);
    if (!bind_group) {
        slog::Error("RHI bind group creation failed: {}", bind_group.error().Message());
        return false;
    }
    bind_group_ = std::move(*bind_group);

    rhi::BindGroupLayout* bind_group_layouts[] = {bind_group_layout_.get()};
    rhi::PipelineLayoutDesc layout_desc{};
    layout_desc.label = "StudioCubePipelineLayout";
    layout_desc.bind_group_layouts = std::span<rhi::BindGroupLayout* const>(bind_group_layouts, 1);
    auto pipeline_layout = device_->CreatePipelineLayout(layout_desc);
    if (!pipeline_layout) {
        slog::Error("RHI pipeline layout creation failed: {}", pipeline_layout.error().Message());
        return false;
    }
    pipeline_layout_ = std::move(*pipeline_layout);

    const std::array vertex_attributes{
        rhi::VertexAttributeDesc{
            .format = rhi::VertexFormat::Float32x3,
            .offset = offsetof(CubeVertex, position),
            .shader_location = 0,
        },
        rhi::VertexAttributeDesc{
            .format = rhi::VertexFormat::Float32x3,
            .offset = offsetof(CubeVertex, color),
            .shader_location = 1,
        },
    };
    const rhi::VertexBufferLayoutDesc vertex_buffer_layout{
        .array_stride = sizeof(CubeVertex),
        .attributes = std::span<const rhi::VertexAttributeDesc>(vertex_attributes.data(), vertex_attributes.size()),
    };
    const rhi::VertexStateDesc vertex_state{
        .module = shader_.get(),
        .entry_point = "vs_main",
        .buffers = std::span<const rhi::VertexBufferLayoutDesc>(&vertex_buffer_layout, 1),
    };

    const rhi::ColorTargetStateDesc color_target{.format = color_format_};
    const rhi::FragmentStateDesc fragment_state{
        .module = shader_.get(),
        .entry_point = "fs_main",
        .targets = std::span<const rhi::ColorTargetStateDesc>(&color_target, 1),
    };

    const rhi::PrimitiveStateDesc primitive_state{
        .topology = rhi::PrimitiveTopology::TriangleList,
        .front_face = rhi::FrontFace::CCW,
        .cull_mode = rhi::CullMode::None,
    };

    const rhi::DepthStencilStateDesc depth_stencil_state{
        .format = rhi::TextureFormat::Depth24PlusStencil8,
        .depth_write_enabled = true,
        .depth_compare = rhi::CompareFunction::Less,
    };

    rhi::RenderPipelineDescTyped pipeline_desc{};
    pipeline_desc.label = "StudioCubePipeline";
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

bool RhiRenderer::UpdateUniforms(const f64 delta_ms) {
    if (uniform_buffer_ == nullptr || device_ == nullptr) {
        return false;
    }

    rotation_seconds_ += static_cast<f32>(delta_ms * 0.001);

    const f32 aspect = height_ != 0 ? static_cast<f32>(width_) / static_cast<f32>(height_) : 1.0f;
    const math::mat4f projection = PerspectiveWebGpu(math::radians(55.0f), aspect, 0.1f, 100.0f);
    const math::mat4f view = math::lookAt(
        math::vec3f{0.0f, 0.0f, 6.0f},
        math::vec3f{0.0f, 0.0f, 0.0f},
        math::vec3f{0.0f, 1.0f, 0.0f});
    const math::mat4f model =
        math::rotate_y(rotation_seconds_ * 0.85f)
        * math::rotate_x(rotation_seconds_ * 0.55f);

    const CubeUniforms uniforms{
        .mvp = projection * view * model,
    };

    if (auto write = device_->GetQueue().WriteBuffer(*uniform_buffer_, 0, &uniforms, sizeof(uniforms)); !write) {
        slog::Warn("RHI uniform update failed: {}", write.error().Message());
        return false;
    }

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

    rhi::DeviceDesc device_desc{};
    device_desc.label = "StudioDevice";
    device_desc.uncaptured_error_callback = [](const rhi::ErrorType type, const std::string_view message) {
        slog::Error("RHI uncaptured device error ({}): {}", static_cast<u32>(type), message);
    };
    auto device = adapter_->CreateDevice(device_desc);
    if (!device) {
        slog::Error("RHI device creation failed: {}", device.error().Message());
        Shutdown();
        return false;
    }
    device_ = std::move(*device);

    rhi::SurfaceCapabilities capabilities{};
    if (auto surface_capabilities = surface_->GetCapabilities(*adapter_, capabilities); !surface_capabilities) {
        slog::Error("RHI surface capability query failed: {}", surface_capabilities.error().Message());
        Shutdown();
        return false;
    }

    if (capabilities.formats.empty()) {
        slog::Error("RHI surface has no supported color formats");
        Shutdown();
        return false;
    }

    const auto preferred_format = std::ranges::find(capabilities.formats, rhi::TextureFormat::BGRA8Unorm);
    color_format_ = preferred_format != capabilities.formats.end()
        ? rhi::TextureFormat::BGRA8Unorm
        : capabilities.formats.front();

    auto swapchain = rhi::Swapchain::Builder(*device_, *surface_)
                         .SizeSource(window_)
                         .ColorFormat(color_format_)
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
    bind_group_.reset();
    bind_group_layout_.reset();
    uniform_buffer_.reset();
    index_buffer_.reset();
    vertex_buffer_.reset();
    pipeline_layout_.reset();
    shader_.reset();
    swapchain_.reset();
    surface_.reset();
    device_.reset();
    adapter_.reset();
    instance_.reset();
    window_ = nullptr;
    width_ = 0;
    height_ = 0;
    color_format_ = rhi::TextureFormat::BGRA8Unorm;
    rotation_seconds_ = 0.0f;
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

bool RhiRenderer::RenderFrame(const f64 delta_ms) {
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

    if (!UpdateUniforms(delta_ms)) {
        return false;
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
    color_attachment.clear_value = rhi::Color{0.42, 0.44, 0.47, 1.0};

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
    pass->SetBindGroup(0, bind_group_.get());
    pass->SetVertexBuffer(0, vertex_buffer_.get());
    pass->SetIndexBuffer(*index_buffer_, rhi::IndexFormat::Uint16);
    pass->DrawIndexed(static_cast<u32>(kCubeIndices.size()));
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
