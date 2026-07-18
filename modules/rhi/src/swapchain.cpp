#include <woki/rhi/device.hpp>
#include <woki/rhi/frame.hpp>
#include <woki/rhi/swapchain.hpp>

#include <woki/window/window.hpp>

namespace woki::rhi {

Frame Swapchain::MakeFrame(
    scope<TextureView> color_view,
    TextureView* depth_view,
    const u32 width,
    const u32 height) {
    Frame frame{};
    frame.color_view_ = std::move(color_view);
    frame.depth_view_ = depth_view;
    frame.width_ = width;
    frame.height_ = height;
    return frame;
}

TextureView& Frame::ColorView() {
    WOKI_ASSERT(color_view_ != nullptr);
    return *color_view_;
}

const TextureView& Frame::ColorView() const {
    WOKI_ASSERT(color_view_ != nullptr);
    return *color_view_;
}

TextureView* Frame::DepthView() noexcept {
    return depth_view_;
}

const TextureView* Frame::DepthView() const noexcept {
    return depth_view_;
}

Swapchain::Builder::Builder(Device& device, Surface& surface)
    : device_(&device)
    , surface_(&surface) {}

Swapchain::Builder& Swapchain::Builder::Size(const u32 width, const u32 height) {
    desc_.width = width;
    desc_.height = height;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::ColorFormat(const TextureFormat format) {
    desc_.format = format;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::DepthFormat(const TextureFormat format) {
    desc_.depth_format = format;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::PresentMode(const enum PresentMode mode) {
    desc_.present_mode = mode;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::AlphaMode(const CompositeAlphaMode alpha_mode) {
    desc_.alpha_mode = alpha_mode;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::EnableDepth(const bool enabled) {
    desc_.enable_depth = enabled;
    return *this;
}

Swapchain::Builder& Swapchain::Builder::Label(const std::string_view label) {
    desc_.label = std::string(label);
    return *this;
}

Swapchain::Builder& Swapchain::Builder::SizeSource(Window* window) noexcept {
    size_source_ = window;
    return *this;
}

Result<scope<Swapchain>> Swapchain::Builder::Build() {
    if (device_ == nullptr || surface_ == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Swapchain builder requires device and surface");
    }

    if ((desc_.width == 0 || desc_.height == 0) && size_source_ != nullptr) {
        desc_.width = size_source_->GetWidth();
        desc_.height = size_source_->GetHeight();
    }

    return device_->CreateSwapchain(*surface_, desc_);
}

} // namespace woki::rhi
