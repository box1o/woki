#include "wgpu_adapter.hpp"

#include "common.hpp"
#include "wgpu_device.hpp"
#include "wgpu_instance.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <memory>

namespace woki::api::wgpu {

namespace {

struct RequestDeviceState {
    bool done{false};
    WGPUDevice device{nullptr};
    std::string message{};
};

struct RequestDeviceContext {
    WGPUInstance instance{nullptr};
    WGPUAdapter adapter{nullptr};
    std::shared_ptr<DeviceLostCallback> device_lost_callback{};
    std::shared_ptr<UncapturedErrorCallback> uncaptured_error_callback{};
    RequestDeviceCallback callback{};
};

void RequestDeviceThunk(WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void*,
    void* userdata) {
    std::unique_ptr<RequestDeviceContext> context(static_cast<RequestDeviceContext*>(userdata));

    scope<Device> device_scope{};
    if (status == WGPURequestDeviceStatus_Success && device != nullptr) {
        device_scope = scope<Device>(new WgpuDeviceImpl(context->instance,
            context->adapter,
            device,
            context->device_lost_callback,
            context->uncaptured_error_callback));
        context->instance = nullptr;
        context->adapter = nullptr;
    }

    context->callback(detail::FromWgpuRequestDeviceStatus(status), std::move(device_scope),
        detail::StringFromView(message));

    if (context->adapter != nullptr) {
        wgpuAdapterRelease(context->adapter);
    }

    if (context->instance != nullptr) {
        wgpuInstanceRelease(context->instance);
    }
}

} // namespace

WgpuAdapterImpl::WgpuAdapterImpl(WGPUInstance instance, WGPUAdapter adapter)
    : instance_(instance)
    , adapter_(adapter)
    , info_(detail::QueryAdapterInfo(adapter_))
    , limits_(detail::QueryLimits(adapter_))
    , features_(detail::QueryFeatures(adapter_)) {}

WgpuAdapterImpl::~WgpuAdapterImpl() {
    instance_view_.reset();

    if (adapter_ != nullptr) {
        wgpuAdapterRelease(adapter_);
        adapter_ = nullptr;
    }

    if (instance_ != nullptr) {
        wgpuInstanceRelease(instance_);
        instance_ = nullptr;
    }
}

scope<Device> WgpuAdapterImpl::CreateDevice(const DeviceDesc& desc) {
    if (adapter_ == nullptr) {
        slog::Error("Cannot create a device from an invalid adapter");
        return nullptr;
    }

#ifdef __EMSCRIPTEN__
    return RequestDevice(desc);
#else
    WOKI_ASSERT(instance_ != nullptr);

    auto storage = detail::BuildDeviceDescriptor(desc);
    WGPUDevice device = wgpuAdapterCreateDevice(adapter_, &storage.native_desc);
    if (device == nullptr) {
        slog::Error("Failed to create WebGPU device '{}'", desc.label);
        return nullptr;
    }

    wgpuInstanceAddRef(instance_);
    wgpuAdapterAddRef(adapter_);
    return scope<Device>(new WgpuDeviceImpl(instance_,
        adapter_,
        device,
        std::move(storage.device_lost_callback),
        std::move(storage.uncaptured_error_callback)));
#endif
}

scope<Device> WgpuAdapterImpl::RequestDevice(const DeviceDesc& desc) {
    if (adapter_ == nullptr) {
        slog::Error("Cannot request a device from an invalid adapter");
        return nullptr;
    }

    auto storage = detail::BuildDeviceDescriptor(desc);

    RequestDeviceState state{};
    WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
#ifdef __EMSCRIPTEN__
    callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
#else
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
#endif
    callback_info.callback = [](WGPURequestDeviceStatus status,
                                WGPUDevice device,
                                WGPUStringView message,
                                void*,
                                void* userdata) {
        auto* state_ptr = static_cast<RequestDeviceState*>(userdata);
        state_ptr->done = true;
        if (status == WGPURequestDeviceStatus_Success) {
            state_ptr->device = device;
        }
        state_ptr->message = detail::StringFromView(message);
    };
    callback_info.userdata2 = &state;

    const auto future = wgpuAdapterRequestDevice(adapter_, &storage.native_desc, callback_info);
    if (future.id == 0) {
        slog::Error("WebGPU device request failed to start");
        return nullptr;
    }

    WOKI_ASSERT(instance_ != nullptr);
    while (!state.done) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#else
        wgpuInstanceProcessEvents(instance_);
#endif
    }

    if (state.device == nullptr) {
        slog::Error("Failed to request WebGPU device: {}",
            state.message.empty() ? std::string{"(no message)"} : state.message);
        return nullptr;
    }

    wgpuInstanceAddRef(instance_);
    wgpuAdapterAddRef(adapter_);
    return scope<Device>(new WgpuDeviceImpl(instance_,
        adapter_,
        state.device,
        std::move(storage.device_lost_callback),
        std::move(storage.uncaptured_error_callback)));
}

Future WgpuAdapterImpl::RequestDevice(
    const DeviceDesc& desc,
    CallbackMode callback_mode,
    RequestDeviceCallback callback) {
    Future future{};
    if (adapter_ == nullptr) {
        future.message = "Adapter is invalid";
        return future;
    }

    if (!callback) {
        future.message = "RequestDevice requires a callback";
        return future;
    }

    auto storage = detail::BuildDeviceDescriptor(desc);

    wgpuInstanceAddRef(instance_);
    wgpuAdapterAddRef(adapter_);

    auto* context = new RequestDeviceContext{
        .instance = instance_,
        .adapter = adapter_,
        .device_lost_callback = storage.device_lost_callback,
        .uncaptured_error_callback = storage.uncaptured_error_callback,
        .callback = std::move(callback),
    };

    WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callback_info.mode = detail::ToWgpuCallbackMode(callback_mode);
    callback_info.callback = RequestDeviceThunk;
    callback_info.userdata2 = context;

    future.id = wgpuAdapterRequestDevice(adapter_, &storage.native_desc, callback_info).id;
    if (future.id == 0) {
        wgpuInstanceRelease(instance_);
        wgpuAdapterRelease(adapter_);
        delete context;
        future.message = "WebGPU device request failed to start";
    }

    return future;
}

Instance* WgpuAdapterImpl::GetInstance() const noexcept {
    if (instance_ == nullptr) {
        return nullptr;
    }

    if (instance_view_ == nullptr) {
        wgpuInstanceAddRef(instance_);
        InstanceDesc desc{};
        desc.backend = Backend::kWebGpu;
        desc.label = "Instance";
        instance_view_ = scope<Instance>(new WgpuInstanceImpl(instance_, desc));
    }

    return instance_view_.get();
}

AdapterInfo WgpuAdapterImpl::GetInfo() const { return info_; }

Status WgpuAdapterImpl::GetInfo(AdapterInfo* info) const { return detail::FillAdapterInfo(adapter_, info); }

Limits WgpuAdapterImpl::GetLimits() const { return limits_; }

Status WgpuAdapterImpl::GetLimits(Limits* limits) const { return detail::FillLimits(adapter_, limits); }

SupportedFeatures WgpuAdapterImpl::GetFeatures() const { return features_; }

bool WgpuAdapterImpl::HasFeature(FeatureName feature) const { return features_.Has(feature); }

NativeHandles WgpuAdapterImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = instance_;
    handles.adapter = adapter_;
    return handles;
}

WGPUAdapter WgpuAdapterImpl::GetWgpuHandle() const noexcept { return adapter_; }

} // namespace woki::api::wgpu
