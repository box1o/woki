#include "wgpu_device.hpp"

#include "common.hpp"
#include "wgpu_adapter.hpp"
#include "wgpu_surface.hpp"
#include "wgpu_swapchain.hpp"

namespace woki::api::wgpu {

WgpuDeviceImpl::WgpuDeviceImpl(WGPUInstance instance,
    WGPUAdapter adapter,
    WGPUDevice device,
    std::shared_ptr<DeviceLostCallback> device_lost_callback,
    std::shared_ptr<UncapturedErrorCallback> uncaptured_error_callback)
    : instance_(instance)
    , adapter_(adapter)
    , device_(device)
    , queue_(device_ != nullptr ? wgpuDeviceGetQueue(device_) : nullptr)
    , adapter_info_(detail::QueryAdapterInfo(adapter_))
    , limits_(detail::QueryLimits(device_))
    , features_(detail::QueryFeatures(device_))
    , device_lost_callback_(std::move(device_lost_callback))
    , uncaptured_error_callback_(std::move(uncaptured_error_callback)) {}

WgpuDeviceImpl::~WgpuDeviceImpl() {
    if (queue_ != nullptr) {
        wgpuQueueRelease(queue_);
        queue_ = nullptr;
    }

    if (device_ != nullptr) {
        wgpuDeviceRelease(device_);
        device_ = nullptr;
    }

    if (adapter_ != nullptr) {
        wgpuAdapterRelease(adapter_);
        adapter_ = nullptr;
    }

    if (instance_ != nullptr) {
        wgpuInstanceRelease(instance_);
        instance_ = nullptr;
    }
}

scope<Adapter> WgpuDeviceImpl::GetAdapter() const {
    if (instance_ == nullptr || adapter_ == nullptr) {
        return nullptr;
    }

    wgpuInstanceAddRef(instance_);
    wgpuAdapterAddRef(adapter_);
    return scope<Adapter>(new WgpuAdapterImpl(instance_, adapter_));
}

AdapterInfo WgpuDeviceImpl::GetAdapterInfo() const { return adapter_info_; }

Status WgpuDeviceImpl::GetAdapterInfo(AdapterInfo* adapter_info) const {
    if (adapter_info == nullptr) {
        return Status::kError;
    }

    *adapter_info = adapter_info_;
    return Status::kSuccess;
}

Limits WgpuDeviceImpl::GetLimits() const { return limits_; }

Status WgpuDeviceImpl::GetLimits(Limits* limits) const {
    return detail::FillLimits(device_, limits);
}

SupportedFeatures WgpuDeviceImpl::GetFeatures() const { return features_; }

bool WgpuDeviceImpl::HasFeature(FeatureName feature) const { return features_.Has(feature); }

NativeHandles WgpuDeviceImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = instance_;
    handles.adapter = adapter_;
    handles.device = device_;
    handles.queue = queue_;
    return handles;
}

void* WgpuDeviceImpl::GetQueueHandle() const noexcept { return queue_; }

scope<Swapchain> WgpuDeviceImpl::CreateSwapchain(const SwapchainDesc& desc) {
    if (desc.surface == nullptr) {
        return nullptr;
    }

    if (dynamic_cast<WgpuSurfaceImpl*>(desc.surface) == nullptr) {
        slog::Error("Cannot create a WebGPU swapchain from a non-WebGPU surface");
        return nullptr;
    }

    WOKI_ASSERT(device_ != nullptr);

    auto swapchain = createScope<WgpuSwapchainImpl>(this, desc);
    if (!swapchain->IsValid()) {
        slog::Error("Failed to create a valid WebGPU swapchain '{}'", desc.label);
        return nullptr;
    }

    return swapchain;
}

void WgpuDeviceImpl::Tick() const noexcept {
#ifndef __EMSCRIPTEN__
    if (device_ != nullptr) {
        wgpuDeviceTick(device_);
    }
#endif

    if (instance_ != nullptr) {
        wgpuInstanceProcessEvents(instance_);
    }
}

void WgpuDeviceImpl::SetLabel(std::string_view label) {
    if (device_ != nullptr) {
        wgpuDeviceSetLabel(device_, detail::ToWgpuStringView(label));
    }
}

WGPUDevice WgpuDeviceImpl::GetWgpuHandle() const noexcept { return device_; }

} // namespace woki::api::wgpu
