#include "wgpu_device.hpp"

#include "detail/device_descriptor.hpp"
#include "wgpu_adapter.hpp"

namespace woki::rhi::wgpu {

WgpuDeviceImpl::WgpuDeviceImpl(
    WgpuAdapterImpl&,
    WGPUInstance instance,
    WGPUAdapter native_adapter,
    WGPUDevice device,
    detail::DeviceDescriptorStorage storage)
    : instance_(instance)
    , adapter_handle_(native_adapter)
    , device_(device)
    , queue_(device != nullptr ? wgpuDeviceGetQueue(device) : nullptr)
    , storage_(std::move(storage)) {
    detail::retain(instance_.get());
    detail::retain(adapter_handle_.get());
}

WgpuDeviceImpl::~WgpuDeviceImpl() {
    queue_.reset();
    device_.reset();
    adapter_handle_.reset();
    instance_.reset();
}

NativeHandles WgpuDeviceImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = instance_.get();
    handles.adapter = adapter_handle_.get();
    handles.device = device_.get();
    handles.queue = queue_.get();
    return handles;
}

void WgpuDeviceImpl::Destroy() {
    if (device_) {
        wgpuDeviceDestroy(device_.get());
    }
}

void WgpuDeviceImpl::Tick() const noexcept {
    if (device_) {
        wgpuDeviceTick(device_.get());
    }
}

void WgpuDeviceImpl::SetLabel(const std::string_view label) {
    if (device_) {
        wgpuDeviceSetLabel(device_.get(), detail::ToStringView(label));
    }
}

WGPUDevice WgpuDeviceImpl::GetNativeDevice() const noexcept {
    return device_.get();
}

} // namespace woki::rhi::wgpu
