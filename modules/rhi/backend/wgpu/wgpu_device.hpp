#pragma once

#include <woki/rhi/device.hpp>

#include "detail/device_descriptor.hpp"
#include "detail/handle.hpp"

namespace woki::rhi::wgpu {

class WgpuAdapterImpl;

class WgpuDeviceImpl final : public Device {
public:
    WgpuDeviceImpl(
        WgpuAdapterImpl& adapter,
        WGPUInstance instance,
        WGPUAdapter native_adapter,
        WGPUDevice device,
        detail::DeviceDescriptorStorage storage);
    ~WgpuDeviceImpl() override;

    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;
    void Destroy() override;
    void Tick() const noexcept override;
    void SetLabel(std::string_view label) override;

    [[nodiscard]] WGPUDevice GetNativeDevice() const noexcept;

private:
    detail::InstanceHandle instance_;
    detail::AdapterHandle adapter_handle_;
    detail::DeviceHandle device_;
    detail::QueueHandle queue_;
    detail::DeviceDescriptorStorage storage_;
};

} // namespace woki::rhi::wgpu
