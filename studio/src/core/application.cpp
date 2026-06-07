#include "application.hpp"
#include "woki/api/instance.hpp"
#include "woki/core.hpp"

#include <vector>

#include <webgpu/webgpu.h>

namespace woki {

namespace {

constexpr double kLogoClearRed = 0x23 / 255.0;
constexpr double kLogoClearGreen = 0xF2 / 255.0;
constexpr double kLogoClearBlue = 0xA1 / 255.0;

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

Application::Application(ApplicationSettings settings) : settings_(std::move(settings)) {
    Initialize();
}

Application::~Application() { Shutdown(); }

void Application::Initialize() {
    WindowOptions options;
    options.title = settings_.title;
    options.width = settings_.width;
    options.height = settings_.height;
    options.floating = settings_.floating;
    options.fullscreen = settings_.fullscreen;
    options.resizable = settings_.resizable;
    options.decorated = settings_.decorated;

    window_ = Window::Create(options);
    if (window_ == nullptr) {
        slog::Critical("Failed to create application window");
        return;
    }

    slog::Info("Created window '{}' ({}x{})", window_->GetTitle(), window_->GetWidth(),
        window_->GetHeight());

    window_event_callback_id_ = window_->AddEventCallback([this](events::Event& event) {
        EmitEvent(event);
    });

    // NOTE: Instance
    instance_ = api::Instance::Create({
        .backend = api::Backend::kAuto,
        .enable_validation = true,
        .enable_debug_labels = true,
    });

    if (instance_ == nullptr) {
        WOKI_PANIC("Failed to create graphics instance");
        return;
    }

    surface_ = instance_->CreateSurface({
        .window = window_.get(),
        .label = "StudioSurface",
    });
    if (surface_ == nullptr) {
        WOKI_PANIC("Failed to create graphics surface");
        return;
    }

    BeginGraphicsInitialization();
}

void Application::BeginGraphicsInitialization() {
    if (graphics_initialization_started_ || instance_ == nullptr || surface_ == nullptr) {
        return;
    }

    graphics_initialization_started_ = true;

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
            scope<api::Adapter> adapter,
            std::string_view message) mutable {
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
                [this](api::RequestDeviceStatus device_status,
                    scope<api::Device> device,
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
                    FinalizeGraphicsInitialization();
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
        WOKI_PANIC("Failed to create graphics device");
        return;
    }

    FinalizeGraphicsInitialization();
#endif
}

void Application::FinalizeGraphicsInitialization() {
    if (device_ == nullptr || surface_ == nullptr || swapchain_ != nullptr) {
        return;
    }

    swapchain_ = device_->CreateSwapchain({
        .surface = surface_.get(),
        .width = window_->GetWidth(),
        .height = window_->GetHeight(),
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
        window_resize_callback_id_ = window_->AddResizeCallback([this](u32 width, u32 height) {
            if (swapchain_ != nullptr) {
                swapchain_->Resize(width, height);
            }
        });
    }

    slog::Info("Created graphics swapchain");
}

void Application::Shutdown() noexcept {
    EmitEvent<events::AppShutdownEvent>();

    if (window_ != nullptr && window_resize_callback_id_ != 0) {
        window_->RemoveResizeCallback(window_resize_callback_id_);
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
    graphics_initialization_started_ = false;
    graphics_initialization_failed_ = false;

    if (window_ != nullptr) {
        window_->RemoveEventCallback(window_event_callback_id_);
        window_->Close();
        window_.reset();
    }

    is_running_ = false;
}

bool Application::IsReady() const noexcept {
    return window_ != nullptr && instance_ != nullptr && surface_ != nullptr;
}

bool Application::IsGraphicsReady() const noexcept {
    return adapter_ != nullptr && device_ != nullptr && swapchain_ != nullptr;
}

void Application::Start() {
    if (!is_running_) {
        slog::Info("Starting application loop");
        frame_timer_.Reset();
        is_running_ = true;
        EmitEvent<events::AppResumeEvent>();
    }
}

void Application::Stop() noexcept {
    if (is_running_) {
        EmitEvent<events::AppSuspendEvent>();
        is_running_ = false;
        slog::Info("Application loop stopped");
    }
}

bool Application::Tick() {
    if (!IsReady()) {
        slog::Critical("Application cannot run without a window");
        return false;
    }

    Start();

    if (!is_running_) {
        return false;
    }

    if (graphics_initialization_failed_) {
        slog::Critical("Graphics initialization failed");
        return false;
    }

    frame_timer_.Tick();
    const auto delta_time = static_cast<f32>(frame_timer_.DeltaSeconds());

    EmitEvent<events::FrameBeginEvent>(delta_time);
    EmitEvent<events::AppTickEvent>(delta_time);

    window_->PollEvents();

    if (!IsGraphicsReady()) {
        if (window_->ShouldClose()) {
            Stop();
            return false;
        }

        return true;
    }

    EmitEvent<events::AppUpdateEvent>(delta_time);
    EmitEvent<events::RenderBeginEvent>();
    EmitEvent<events::AppRenderEvent>();

    RenderClearPass(*device_, *swapchain_);

    EmitEvent<events::RenderEndEvent>();
    EmitEvent<events::SwapBuffersEvent>();
    EmitEvent<events::FrameEndEvent>();

    // NOTE: Drawing and update logic would go here

    if (window_->ShouldClose()) {
        Stop();
        return false;
    }

    return true;
}

void Application::Run() { while (Tick()); }

CallbackId Application::AddEventCallback(EventCallback callback) {
    const CallbackId callback_id = next_callback_id_++;
    event_callbacks_.emplace(callback_id, std::move(callback));
    return callback_id;
}

void Application::RemoveEventCallback(CallbackId id) { event_callbacks_.erase(id); }

void Application::EmitEvent(events::Event& event) {
    event.timestamp = Clock::Seconds();

    std::vector<EventCallback> callbacks;
    callbacks.reserve(event_callbacks_.size());
    for (const auto& [id, callback] : event_callbacks_) {
        (void)id;
        if (callback) {
            callbacks.push_back(callback);
        }
    }

    for (const auto& callback : callbacks) {
        callback(event);
    }
}

} // namespace woki
