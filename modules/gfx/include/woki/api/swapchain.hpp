#pragma once

#include "woki/api/descriptors.hpp"

#include <woki/core.hpp>

namespace woki::api {

class Swapchain {
public:
    virtual ~Swapchain() = default;

    [[nodiscard]] virtual TextureFormat GetColorFormat() const noexcept = 0;
    [[nodiscard]] virtual TextureFormat GetDepthFormat() const noexcept = 0;
    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;
    virtual void Resize(u32 width, u32 height) noexcept = 0;
    [[nodiscard]] virtual Result<SwapchainFrame> AcquireNextFrame() noexcept = 0;
    [[nodiscard]] virtual Result<void> Present() noexcept = 0;

protected:
    Swapchain() = default;
};

} // namespace woki::api
