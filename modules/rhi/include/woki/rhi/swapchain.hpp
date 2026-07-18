#pragma once

#include "frame.hpp"
#include "swapchain_desc.hpp"

#include <string_view>

#include <woki/core.hpp>

namespace woki::rhi {

class Device;
class Surface;
class TextureView;

class Swapchain {
public:
    class Builder;

    virtual ~Swapchain() = default;

    [[nodiscard]] virtual TextureFormat ColorFormat() const noexcept = 0;
    [[nodiscard]] virtual TextureFormat DepthFormat() const noexcept = 0;
    [[nodiscard]] virtual u32 Width() const noexcept = 0;
    [[nodiscard]] virtual u32 Height() const noexcept = 0;

    virtual void Resize(u32 width, u32 height) = 0;
    [[nodiscard]] virtual Result<Frame> AcquireNextFrame() = 0;
    [[nodiscard]] virtual Result<void> Present() = 0;

protected:
    Swapchain() = default;

    [[nodiscard]] static Frame MakeFrame(
        scope<TextureView> color_view, TextureView* depth_view, u32 width, u32 height);
};

class Swapchain::Builder {
public:
    Builder(Device& device, Surface& surface);

    Builder& Size(u32 width, u32 height);
    Builder& ColorFormat(TextureFormat format);
    Builder& DepthFormat(TextureFormat format);
    Builder& PresentMode(PresentMode mode);
    Builder& AlphaMode(CompositeAlphaMode alpha_mode);
    Builder& EnableDepth(bool enabled);
    Builder& Label(std::string_view label);

    [[nodiscard]] Result<scope<Swapchain>> Build();

private:
    Device* device_{nullptr};
    Surface* surface_{nullptr};
    SwapchainDesc desc_{};
};

} // namespace woki::rhi
