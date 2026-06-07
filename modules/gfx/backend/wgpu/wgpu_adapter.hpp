#pragma once

#include "../../include/woki/api/adapter.hpp"

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuInstanceImpl;

class WgpuAdapterImpl final : public Adapter {
public:
    WgpuAdapterImpl(WGPUInstance instance, WGPUAdapter adapter);
    ~WgpuAdapterImpl() override;

    [[nodiscard]] scope<Device> CreateDevice(const DeviceDesc& desc = {}) override;
    [[nodiscard]] scope<Device> RequestDevice(const DeviceDesc& desc = {}) override;
    [[nodiscard]] Future RequestDevice(
        const DeviceDesc& desc,
        CallbackMode callback_mode,
        RequestDeviceCallback callback) override;
    [[nodiscard]] Instance* GetInstance() const noexcept override;
    [[nodiscard]] AdapterInfo GetInfo() const override;
    [[nodiscard]] Status GetInfo(AdapterInfo* info) const override;
    [[nodiscard]] Limits GetLimits() const override;
    [[nodiscard]] Status GetLimits(Limits* limits) const override;
    [[nodiscard]] SupportedFeatures GetFeatures() const override;
    [[nodiscard]] bool HasFeature(FeatureName feature) const override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;

    [[nodiscard]] WGPUAdapter GetWgpuHandle() const noexcept;

private:
    WGPUInstance instance_{nullptr};
    WGPUAdapter adapter_{nullptr};
    AdapterInfo info_{};
    Limits limits_{};
    SupportedFeatures features_{};
    mutable scope<Instance> instance_view_{};
};

} // namespace woki::api::wgpu
