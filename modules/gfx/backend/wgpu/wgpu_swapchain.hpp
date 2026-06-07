#pragma once

#include "../../include/woki/api/swapchain.hpp"

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuDeviceImpl;
class WgpuSurfaceImpl;

class WgpuSwapchainImpl final : public Swapchain {
public:
    WgpuSwapchainImpl(WgpuDeviceImpl* device, const SwapchainDesc& desc);
    ~WgpuSwapchainImpl() override;

    [[nodiscard]] bool IsValid() const noexcept;

    [[nodiscard]] TextureFormat GetColorFormat() const noexcept override;
    [[nodiscard]] TextureFormat GetDepthFormat() const noexcept override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;
    void Resize(u32 width, u32 height) noexcept override;
    [[nodiscard]] Result<SwapchainFrame> AcquireNextFrame() noexcept override;
    [[nodiscard]] Result<void> Present() noexcept override;

private:
    bool Configure() noexcept;
    bool CreateOrResizeDepth() noexcept;
    void ReleaseDepthResources() noexcept;

    WgpuDeviceImpl* device_{nullptr};
    WgpuSurfaceImpl* surface_{nullptr};
    SwapchainDesc desc_{};
    u32 width_{0};
    u32 height_{0};
    WGPUTexture depth_texture_{nullptr};
    WGPUTextureView depth_view_{nullptr};
    bool valid_{false};
};

} // namespace woki::api::wgpu
