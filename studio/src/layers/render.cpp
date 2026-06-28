#include "render.hpp"

#include "woki/api/instance.hpp"

#include <webgpu/webgpu.h>

namespace woki {

namespace {

constexpr double kLogoClearRed = 0x18 / 255.0;
constexpr double kLogoClearGreen = 0x18 / 255.0;
constexpr double kLogoClearBlue = 0x18 / 255.0;

void RenderClearPass(api::Device& device, api::Swapchain& swapchain) {
    auto frame = swapchain.AcquireNextFrame();
    if (!frame) {
        frame.error().Log();
        return;
    }

    const auto device_handles = device.GetNativeHandles();
    auto* native_device = static_cast<WGPUDevice>(device_handles.device);
    auto* native_queue = static_cast<WGPUQueue>(device_handles.queue);
    auto* color_view = static_cast<WGPUTextureView>(frame->color.resource);
    auto* depth_view = static_cast<WGPUTextureView>(frame->depth.resource);
    WOKI_ASSERT(native_device != nullptr);
    WOKI_ASSERT(native_queue != nullptr);
    WOKI_ASSERT(color_view != nullptr);
    if (native_device == nullptr || native_queue == nullptr || color_view == nullptr) {
        slog::Error("Missing native WebGPU handles for clear pass");
        return;
    }

    WGPUCommandEncoderDescriptor encoder_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    encoder_desc.label = {"StudioClearEncoder", WGPU_STRLEN};
    auto* encoder = wgpuDeviceCreateCommandEncoder(native_device, &encoder_desc);
    if (encoder == nullptr) {
        slog::Error("Failed to create WebGPU command encoder");
        return;
    }

    WGPURenderPassColorAttachment color_attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color_attachment.view = color_view;
    color_attachment.loadOp = WGPULoadOp_Clear;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = {kLogoClearRed, kLogoClearGreen, kLogoClearBlue, 1.0};

    WGPURenderPassDepthStencilAttachment depth_attachment =
        WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    WGPURenderPassDescriptor pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    pass_desc.label = {"StudioClearPass", WGPU_STRLEN};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attachment;
    if (depth_view != nullptr) {
        depth_attachment.view = depth_view;
        depth_attachment.depthLoadOp = WGPULoadOp_Clear;
        depth_attachment.depthStoreOp = WGPUStoreOp_Store;
        depth_attachment.depthClearValue = 1.0f;
        depth_attachment.depthReadOnly = false;
        depth_attachment.stencilLoadOp = WGPULoadOp_Clear;
        depth_attachment.stencilStoreOp = WGPUStoreOp_Store;
        depth_attachment.stencilClearValue = 0;
        depth_attachment.stencilReadOnly = false;
        pass_desc.depthStencilAttachment = &depth_attachment;
    }

    auto* pass = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);
    if (pass == nullptr) {
        wgpuCommandEncoderRelease(encoder);
        slog::Error("Failed to begin WebGPU render pass");
        return;
    }

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor command_buffer_desc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    command_buffer_desc.label = {"StudioClearCommands", WGPU_STRLEN};
    auto* command_buffer = wgpuCommandEncoderFinish(encoder, &command_buffer_desc);
    wgpuCommandEncoderRelease(encoder);
    if (command_buffer == nullptr) {
        slog::Error("Failed to finish WebGPU command buffer");
        return;
    }

    wgpuQueueSubmit(native_queue, 1, &command_buffer);
    wgpuCommandBufferRelease(command_buffer);

    if (auto present = swapchain.Present(); !present) {
        present.error().Log();
    }
}

} // namespace

void RenderLayer::OnAttach(Context& ctx) {
    window_ = ctx.window;
    BeginGraphicsInitialization(ctx.window);
}

void RenderLayer::OnDetach(Context& ctx) {
    if (window_resize_callback_id_ != 0) {
        ctx.window.RemoveResizeCallback(window_resize_callback_id_);
        window_resize_callback_id_ = 0;
    }

    swapchain_.reset();
    if (surface_ != nullptr) {
        surface_->Unconfigure();
    }
    surface_.reset();
    device_.reset();
    adapter_.reset();
    instance_.reset();
    window_.reset();
    graphics_initialization_started_ = false;
    graphics_initialization_failed_ = false;
}

void RenderLayer::OnUpdate(Context& ctx, f64 delta_ms) {
    (void)delta_ms;
    if (graphics_initialization_failed_) {
        slog::Critical("Graphics initialization failed");
        ctx.running = false;
        return;
    }

    if (!IsGraphicsReady()) {
        return;
    }

    RenderClearPass(*device_, *swapchain_);
}

void RenderLayer::BeginGraphicsInitialization(Window& window) {
    if (graphics_initialization_started_) {
        return;
    }

    graphics_initialization_started_ = true;

    instance_ = api::Instance::Create({
        .backend = api::Backend::kAuto,
        .enable_validation = true,
        .enable_debug_labels = true,
    });
    if (instance_ == nullptr) {
        graphics_initialization_failed_ = true;
        WOKI_PANIC("Failed to create graphics instance");
        return;
    }

    surface_ = instance_->CreateSurface({
        .window = &window,
        .label = "StudioSurface",
    });
    if (surface_ == nullptr) {
        graphics_initialization_failed_ = true;
        WOKI_PANIC("Failed to create graphics surface");
        return;
    }

#ifdef __EMSCRIPTEN__
    auto device_desc = api::DeviceDesc{
        .backend = api::Backend::kWebGpu,
        .enable_validation = true,
        .enable_debug_labels = true,
        .label = "StudioDevice",
    };

    auto adapter_desc = api::RequestAdapterDesc{
        .compatible_surface = surface_.get(),
        .power_preference = api::PowerPreference::kHighPerformance,
    };

    auto future = instance_->RequestAdapter(adapter_desc, api::CallbackMode::kAllowSpontaneous,
        [this, device_desc = std::move(device_desc)](api::RequestAdapterStatus status,
            scope<api::Adapter> adapter, std::string_view message) mutable {
            if (status != api::RequestAdapterStatus::kSuccess || adapter == nullptr) {
                graphics_initialization_failed_ = true;
                slog::Critical("Failed to request graphics adapter: {}",
                    message.empty() ? std::string{"(no message)"} : std::string(message));
                return;
            }

            adapter_ = std::move(adapter);
            slog::Info("Acquired graphics adapter");

            auto device_future = adapter_->RequestDevice(device_desc,
                api::CallbackMode::kAllowSpontaneous,
                [this](api::RequestDeviceStatus device_status, scope<api::Device> device,
                    std::string_view device_message) {
                    if (device_status != api::RequestDeviceStatus::kSuccess || device == nullptr) {
                        graphics_initialization_failed_ = true;
                        slog::Critical("Failed to create graphics device: {}",
                            device_message.empty() ? std::string{"(no message)"}
                                                   : std::string(device_message));
                        return;
                    }

                    device_ = std::move(device);
                    slog::Info("Created graphics device");
                    if (window_.has_value()) {
                        FinalizeGraphicsInitialization(window_->get());
                    }
                });

            if (device_future.id == 0) {
                graphics_initialization_failed_ = true;
                slog::Critical("Failed to start graphics device request");
            }
        });

    if (future.id == 0) {
        graphics_initialization_failed_ = true;
        slog::Critical("Failed to start graphics adapter request");
    }
#else
    adapter_ = instance_->RequestAdapter({
        .compatible_surface = surface_.get(),
        .power_preference = api::PowerPreference::kHighPerformance,
    });
    if (adapter_ == nullptr) {
        graphics_initialization_failed_ = true;
        WOKI_PANIC("Failed to request graphics adapter");
        return;
    }

    device_ = adapter_->CreateDevice({
        .backend = api::Backend::kWebGpu,
        .enable_validation = true,
        .enable_debug_labels = true,
        .label = "StudioDevice",
    });
    if (device_ == nullptr) {
        graphics_initialization_failed_ = true;
        WOKI_PANIC("Failed to create graphics device");
        return;
    }

    FinalizeGraphicsInitialization(window);
#endif
}

void RenderLayer::FinalizeGraphicsInitialization(Window& window) {
    if (device_ == nullptr || surface_ == nullptr || swapchain_ != nullptr) {
        return;
    }

    swapchain_ = device_->CreateSwapchain({
        .surface = surface_.get(),
        .width = window.GetWidth(),
        .height = window.GetHeight(),
        .format = api::TextureFormat::kBgra8Unorm,
        .depth_format = api::TextureFormat::kDepth24PlusStencil8,
        .usage = api::TextureUsage::kRenderAttachment,
        .present_mode = api::PresentMode::kFifo,
        .alpha_mode = api::CompositeAlphaMode::kAuto,
        .enable_depth = true,
        .label = "StudioSwapchain",
    });
    if (swapchain_ == nullptr) {
        graphics_initialization_failed_ = true;
        WOKI_PANIC("Failed to create graphics swapchain");
        return;
    }

    if (window_resize_callback_id_ == 0) {
        window_resize_callback_id_ = window.AddResizeCallback([this](u32 width, u32 height) {
            if (swapchain_ != nullptr) {
                swapchain_->Resize(width, height);
            }
        });
    }

    slog::Info("Created graphics swapchain");
}

bool RenderLayer::IsGraphicsReady() const noexcept {
    return adapter_ != nullptr && device_ != nullptr && swapchain_ != nullptr;
}

} // namespace woki
