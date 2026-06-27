#pragma once

#include "../../include/woki/api/device.hpp"

#include <memory>

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuAdapterImpl;

class WgpuDeviceImpl final : public Device {
public:
    WgpuDeviceImpl(WGPUInstance instance,
        WGPUAdapter adapter,
        WGPUDevice device,
        std::shared_ptr<DeviceLostCallback> device_lost_callback = {},
        std::shared_ptr<UncapturedErrorCallback> uncaptured_error_callback = {});
    ~WgpuDeviceImpl() override;

    [[nodiscard]] scope<Adapter> GetAdapter() const override;
    [[nodiscard]] AdapterInfo GetAdapterInfo() const override;
    [[nodiscard]] Status GetAdapterInfo(AdapterInfo* adapter_info) const override;
    [[nodiscard]] Limits GetLimits() const override;
    [[nodiscard]] Status GetLimits(Limits* limits) const override;
    [[nodiscard]] SupportedFeatures GetFeatures() const override;
    [[nodiscard]] bool HasFeature(FeatureName feature) const override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;
    [[nodiscard]] void* GetQueueHandle() const noexcept override;
    [[nodiscard]] scope<Swapchain> CreateSwapchain(const SwapchainDesc& desc) override;
    void Tick() const noexcept override;
    void SetLabel(std::string_view label) override;

    [[nodiscard]] WGPUDevice GetWgpuHandle() const noexcept;

private:
    WGPUInstance instance_{nullptr};
    WGPUAdapter adapter_{nullptr};
    WGPUDevice device_{nullptr};
    WGPUQueue queue_{nullptr};
    AdapterInfo adapter_info_{};
    Limits limits_{};
    SupportedFeatures features_{};
    std::shared_ptr<DeviceLostCallback> device_lost_callback_{};
    std::shared_ptr<UncapturedErrorCallback> uncaptured_error_callback_{};
};

} // namespace woki::api::wgpu
