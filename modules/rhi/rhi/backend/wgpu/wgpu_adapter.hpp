#pragma once

#include <woki/rhi/adapter.hpp>

#include "detail/handle.hpp"

namespace woki::rhi::wgpu {

class WgpuInstanceImpl;

class WgpuAdapterImpl final : public Adapter {
public:
    WgpuAdapterImpl(WgpuInstanceImpl& instance, WGPUAdapter adapter);
    ~WgpuAdapterImpl() override;

    [[nodiscard]] Result<scope<Device>> CreateDevice(const DeviceDesc& desc = {}) override;
    [[nodiscard]] Result<scope<Device>> RequestDevice(const DeviceDesc& desc = {}) override;
    [[nodiscard]] Future RequestDevice(
        const DeviceDesc& desc,
        CallbackMode callback_mode,
        RequestDeviceCallback callback) override;

    [[nodiscard]] Instance& GetInstance() const noexcept override;

    [[nodiscard]] AdapterInfo GetInfo() const override;
    [[nodiscard]] Result<void> GetInfo(AdapterInfo& info) const override;

    void GetFeatures(SupportedFeatures& features) const override;
    [[nodiscard]] SupportedFeatures GetFeatures() const override;

    [[nodiscard]] Result<void> GetFormatCapabilities(
        TextureFormat format, DawnFormatCapabilities& capabilities) const override;

    [[nodiscard]] Limits GetLimits() const override;
    [[nodiscard]] Result<void> GetLimits(Limits& limits) const override;

    [[nodiscard]] bool HasFeature(FeatureName feature) const noexcept override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;

    [[nodiscard]] WGPUAdapter GetNativeAdapter() const noexcept;
    [[nodiscard]] WGPUInstance GetNativeInstance() const noexcept;

private:
    WgpuInstanceImpl* instance_{nullptr};
    detail::InstanceHandle instance_handle_;
    detail::AdapterHandle adapter_;
    AdapterInfo info_;
    SupportedFeatures features_;
    Limits limits_;
};

} // namespace woki::rhi::wgpu
