#include "rhi_renderer.hpp"

#include <woki/rhi.hpp>

namespace woki {

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

    ready_ = true;
    slog::Info("RHI renderer initialized ({}x{})", width_, height_);
    return true;
}

void RhiRenderer::Shutdown() noexcept {
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
    pass_desc.label = "StudioClearPass";
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
