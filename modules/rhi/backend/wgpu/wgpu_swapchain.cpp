#include "wgpu_swapchain.hpp"

#include "detail/string.hpp"
#include "wgpu_device.hpp"
#include "wgpu_enums.hpp"
#include "wgpu_objects.hpp"
#include "wgpu_surface.hpp"

#include <woki/rhi/device.hpp>

namespace woki::rhi::wgpu {
namespace {

using convert::ToWgpu;

} // namespace

WgpuSwapchainImpl::WgpuSwapchainImpl(
    WgpuDeviceImpl& device,
    WgpuSurfaceImpl& surface,
    SwapchainDesc desc)
    : device_(&device)
    , surface_(&surface)
    , desc_(std::move(desc))
    , width_(desc_.width)
    , height_(desc_.height) {
    if (auto result = Configure(); !result) {
        slog::Error("Failed to configure swapchain '{}': {}", desc_.label, result.error().Message());
        return;
    }
    if (auto result = CreateOrResizeDepth(); !result) {
        slog::Error("Failed to create swapchain depth '{}': {}", desc_.label, result.error().Message());
    }
}

WgpuSwapchainImpl::~WgpuSwapchainImpl() {
    ReleaseDepthResources();
    if (surface_ != nullptr) {
        (void)surface_->Unconfigure();
    }
}

TextureFormat WgpuSwapchainImpl::ColorFormat() const noexcept {
    return desc_.format;
}

TextureFormat WgpuSwapchainImpl::DepthFormat() const noexcept {
    return desc_.depth_format;
}

u32 WgpuSwapchainImpl::Width() const noexcept {
    return width_;
}

u32 WgpuSwapchainImpl::Height() const noexcept {
    return height_;
}

void WgpuSwapchainImpl::Resize(const u32 width, const u32 height) {
    if (width == 0 || height == 0 || (width == width_ && height == height_)) {
        return;
    }

    width_ = width;
    height_ = height;
    desc_.width = width;
    desc_.height = height;

    if (auto result = Configure(); !result) {
        slog::Error("Failed to reconfigure swapchain '{}': {}", desc_.label, result.error().Message());
        return;
    }
    if (auto result = CreateOrResizeDepth(); !result) {
        slog::Error("Failed to resize swapchain depth '{}': {}", desc_.label, result.error().Message());
    }
}

Result<Frame> WgpuSwapchainImpl::AcquireNextFrame() {
    if (surface_ == nullptr || device_ == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Swapchain is invalid");
    }

    SurfaceTexture surface_texture{};
    surface_->GetCurrentTexture(surface_texture);

    if (surface_texture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal
        && surface_texture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to acquire surface texture");
    }

    if (surface_texture.handles.resource == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Surface texture view is null");
    }

    auto color_view = CreateTextureViewObject(
        static_cast<WGPUTextureView>(surface_texture.handles.resource));
    if (!color_view) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to wrap surface texture view");
    }

    return Ok(MakeFrame(
        std::move(color_view),
        desc_.enable_depth ? depth_view_.get() : nullptr,
        width_,
        height_));
}

Result<void> WgpuSwapchainImpl::Present() {
    if (surface_ == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Swapchain surface is invalid");
    }

    return surface_->Present();
}

Result<void> WgpuSwapchainImpl::Configure() {
    if (surface_ == nullptr || device_ == nullptr || width_ == 0 || height_ == 0) {
        return Err(ErrorCode::GraphicsFramebufferIncomplete, "Swapchain configure prerequisites missing");
    }

    SurfaceConfiguration config{};
    config.device = device_;
    config.format = desc_.format;
    config.usage = desc_.usage;
    config.width = width_;
    config.height = height_;
    config.alpha_mode = desc_.alpha_mode;
    config.present_mode = desc_.present_mode;

    return surface_->Configure(config);
}

Result<void> WgpuSwapchainImpl::CreateOrResizeDepth() {
    depth_view_.reset();
    depth_texture_.reset();

    if (!desc_.enable_depth || device_ == nullptr || width_ == 0 || height_ == 0) {
        return Ok();
    }

    const WGPUDevice native_device = device_->GetNativeDevice();
    if (native_device == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Device is invalid");
    }

    WGPUTextureDescriptor texture_desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.size = WGPUExtent3D{width_, height_, 1};
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.format = ToWgpu(desc_.depth_format);
    texture_desc.usage = WGPUTextureUsage_RenderAttachment;
    texture_desc.label = detail::ToStringView(desc_.label + "Depth");

    WGPUTexture native_texture = wgpuDeviceCreateTexture(native_device, &texture_desc);
    if (native_texture == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to create swapchain depth texture");
    }

    WGPUTextureView native_view = wgpuTextureCreateView(native_texture, nullptr);
    if (native_view == nullptr) {
        wgpuTextureRelease(native_texture);
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to create swapchain depth view");
    }

    depth_texture_.reset(native_texture);
    depth_view_ = CreateTextureViewObject(native_view);
    if (!depth_view_) {
        depth_texture_.reset();
        wgpuTextureViewRelease(native_view);
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to wrap swapchain depth view");
    }

    return Ok();
}

void WgpuSwapchainImpl::ReleaseDepthResources() noexcept {
    depth_view_.reset();
    depth_texture_.reset();
}

Result<scope<Swapchain>> CreateSwapchainObject(
    WgpuDeviceImpl& device,
    Surface& surface,
    SwapchainDesc desc) {
    auto* wgpu_surface = dynamic_cast<WgpuSurfaceImpl*>(&surface);
    if (wgpu_surface == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Surface backend mismatch");
    }

    if (desc.width == 0 || desc.height == 0) {
        return Err(ErrorCode::GraphicsFramebufferIncomplete,
            "SwapchainDesc width and height must be non-zero (use Swapchain::Builder::SizeSource for window size)");
    }

    return Ok(createScope<WgpuSwapchainImpl>(device, *wgpu_surface, std::move(desc)));
}

} // namespace woki::rhi::wgpu
