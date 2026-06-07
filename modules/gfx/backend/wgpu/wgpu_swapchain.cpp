#include "wgpu_swapchain.hpp"

#include "common.hpp"
#include "wgpu_device.hpp"
#include "wgpu_surface.hpp"

#include "../../include/woki/window/window.hpp"

namespace woki::api::wgpu {

WgpuSwapchainImpl::WgpuSwapchainImpl(WgpuDeviceImpl* device, const SwapchainDesc& desc)
    : device_(device)
    , surface_(desc.surface != nullptr ? dynamic_cast<WgpuSurfaceImpl*>(desc.surface) : nullptr)
    , desc_(desc)
    , width_(desc.width)
    , height_(desc.height) {
    if ((width_ == 0 || height_ == 0) && surface_ != nullptr && surface_->GetWindow() != nullptr) {
        width_ = surface_->GetWindow()->GetWidth();
        height_ = surface_->GetWindow()->GetHeight();
    }

    WOKI_ASSERT(device_ != nullptr);
    WOKI_ASSERT(surface_ != nullptr);

    if (!Configure()) {
        slog::Error("Failed to configure swapchain '{}'", desc_.label);
        return;
    }
    if (!CreateOrResizeDepth()) {
        slog::Error("Failed to create swapchain depth resources '{}'", desc_.label);
        return;
    }

    valid_ = true;
}

WgpuSwapchainImpl::~WgpuSwapchainImpl() {
    ReleaseDepthResources();
}

bool WgpuSwapchainImpl::IsValid() const noexcept { return valid_; }

TextureFormat WgpuSwapchainImpl::GetColorFormat() const noexcept { return desc_.format; }

TextureFormat WgpuSwapchainImpl::GetDepthFormat() const noexcept { return desc_.depth_format; }

NativeHandles WgpuSwapchainImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    if (surface_ != nullptr) {
        handles = surface_->GetNativeHandles();
    }
    handles.device = device_ != nullptr ? device_->GetNativeHandles().device : nullptr;
    handles.resource =
        depth_view_ != nullptr ? static_cast<void*>(depth_view_) : static_cast<void*>(depth_texture_);
    return handles;
}

void WgpuSwapchainImpl::Resize(u32 width, u32 height) noexcept {
    if (width == 0 || height == 0) {
        return;
    }

    if (width == width_ && height == height_) {
        return;
    }

    width_ = width;
    height_ = height;
    valid_ = false;
    if (!Configure()) {
        slog::Error("Failed to reconfigure swapchain '{}'", desc_.label);
        return;
    }
    if (!CreateOrResizeDepth()) {
        slog::Error("Failed to recreate swapchain depth resources '{}'", desc_.label);
        return;
    }

    valid_ = true;
}

Result<SwapchainFrame> WgpuSwapchainImpl::AcquireNextFrame() noexcept {
    if (surface_ == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Swapchain surface is null");
    }

    WOKI_ASSERT(device_ != nullptr);

    const auto surface_texture = surface_->GetCurrentTexture();
    if (surface_texture.status != SurfaceGetCurrentTextureStatus::kSuccessOptimal
        && surface_texture.status != SurfaceGetCurrentTextureStatus::kSuccessSuboptimal) {
        return Err(ErrorCode::GraphicsResourceCreationFailed,
            "Failed to acquire current surface texture");
    }

    if (surface_texture.handles.resource == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed,
            "Failed to acquire current surface texture");
    }

    SwapchainFrame frame{};
    frame.color = surface_texture.handles;
    frame.width = width_;
    frame.height = height_;
    if (desc_.enable_depth && depth_view_ != nullptr) {
        frame.depth.device = device_ != nullptr ? device_->GetNativeHandles().device : nullptr;
        frame.depth.resource = depth_view_;
    }

    return Ok(std::move(frame));
}

Result<void> WgpuSwapchainImpl::Present() noexcept {
    if (surface_ == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Swapchain surface is null");
    }

    if (surface_->Present() != Status::kSuccess) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Surface present failed");
    }

    return Ok();
}

bool WgpuSwapchainImpl::Configure() noexcept {
    if (surface_ == nullptr || device_ == nullptr || width_ == 0 || height_ == 0) {
        return false;
    }

    WOKI_ASSERT(desc_.surface == surface_);

    SurfaceConfiguration config{};
    config.device = device_;
    config.format = desc_.format;
    config.usage = desc_.usage;
    config.width = width_;
    config.height = height_;
    config.alpha_mode = desc_.alpha_mode;
    config.present_mode = desc_.present_mode;
    surface_->Configure(config);
    return true;
}

bool WgpuSwapchainImpl::CreateOrResizeDepth() noexcept {
    ReleaseDepthResources();
    if (!desc_.enable_depth || device_ == nullptr || width_ == 0 || height_ == 0) {
        return true;
    }

    WOKI_ASSERT(device_->GetWgpuHandle() != nullptr);

    WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size = WGPUExtent3D{width_, height_, 1};
    descriptor.mipLevelCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = detail::ToWgpuTextureFormat(desc_.depth_format);
    descriptor.usage = WGPUTextureUsage_RenderAttachment;
    descriptor.label = detail::ToWgpuStringView("SwapchainDepth");

    depth_texture_ = wgpuDeviceCreateTexture(device_->GetWgpuHandle(), &descriptor);
    if (depth_texture_ == nullptr) {
        return false;
    }

    depth_view_ = wgpuTextureCreateView(depth_texture_, nullptr);
    if (depth_view_ == nullptr) {
        wgpuTextureRelease(depth_texture_);
        depth_texture_ = nullptr;
        return false;
    }

    return true;
}

void WgpuSwapchainImpl::ReleaseDepthResources() noexcept {
    if (depth_view_ != nullptr) {
        wgpuTextureViewRelease(depth_view_);
        depth_view_ = nullptr;
    }

    if (depth_texture_ != nullptr) {
        wgpuTextureRelease(depth_texture_);
        depth_texture_ = nullptr;
    }
}

} // namespace woki::api::wgpu
