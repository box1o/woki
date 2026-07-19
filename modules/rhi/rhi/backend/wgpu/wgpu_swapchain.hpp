#pragma once

#include <woki/rhi/swapchain.hpp>

#include "detail/handle.hpp"

namespace woki::rhi::wgpu {

class WgpuDeviceImpl;
class WgpuSurfaceImpl;

class WgpuSwapchainImpl final : public Swapchain {
public:
    WgpuSwapchainImpl(WgpuDeviceImpl& device, WgpuSurfaceImpl& surface, SwapchainDesc desc);
    ~WgpuSwapchainImpl() override;

    [[nodiscard]] TextureFormat ColorFormat() const noexcept override;
    [[nodiscard]] TextureFormat DepthFormat() const noexcept override;
    [[nodiscard]] u32 Width() const noexcept override;
    [[nodiscard]] u32 Height() const noexcept override;

    void Resize(u32 width, u32 height) override;
    [[nodiscard]] Result<Frame> AcquireNextFrame() override;
    [[nodiscard]] Result<void> Present() override;

private:
    [[nodiscard]] Result<void> Configure();
    [[nodiscard]] Result<void> CreateOrResizeDepth();
    void ReleaseDepthResources() noexcept;

    WgpuDeviceImpl* device_{nullptr};
    WgpuSurfaceImpl* surface_{nullptr};
    SwapchainDesc desc_{};
    u32 width_{0};
    u32 height_{0};
    detail::TextureHandle depth_texture_;
    scope<TextureView> depth_view_;
};

[[nodiscard]] Result<scope<Swapchain>> CreateSwapchainObject(
    WgpuDeviceImpl& device,
    Surface& surface,
    SwapchainDesc desc);

} // namespace woki::rhi::wgpu
