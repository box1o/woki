#include "wgpu_adapter.hpp"

#include "detail/adapter_info.hpp"
#include "detail/device_descriptor.hpp"
#include "detail/limits.hpp"
#include "detail/string.hpp"
#include "wgpu_device.hpp"
#include "wgpu_enums.hpp"
#include "wgpu_instance.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <utility>

namespace woki::rhi::wgpu {
namespace {

using convert::FromWgpu;
using convert::ToWgpu;

struct RequestDeviceCallbackState {
    std::function<void(WGPURequestDeviceStatus, WGPUDevice, std::string_view)> callback;
};

void RequestDeviceThunk(WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void* userdata1,
    void*) {
    auto state = scope<RequestDeviceCallbackState>(
        static_cast<RequestDeviceCallbackState*>(userdata1));
    if (state == nullptr || !state->callback) {
        return;
    }

    state->callback(status, device, detail::StringFromView(message));
}

[[nodiscard]] scope<Device> CreateDeviceFromNative(
    WgpuAdapterImpl& adapter,
    WGPUDevice device,
    detail::DeviceDescriptorStorage storage) {
    if (device == nullptr) {
        return nullptr;
    }

    detail::retain(adapter.GetNativeInstance());
    detail::retain(adapter.GetNativeAdapter());
    return createScope<WgpuDeviceImpl>(
        adapter,
        adapter.GetNativeInstance(),
        adapter.GetNativeAdapter(),
        device,
        std::move(storage));
}

} // namespace

WgpuAdapterImpl::WgpuAdapterImpl(WgpuInstanceImpl& instance, WGPUAdapter adapter)
    : instance_(&instance)
    , instance_handle_(instance.GetNativeInstance())
    , adapter_(adapter)
    , info_(detail::QueryAdapterInfo(adapter))
    , features_(detail::QuerySupportedFeatures(adapter))
    , limits_(detail::QueryLimits(adapter)) {
    detail::retain(instance_handle_.get());
}

WgpuAdapterImpl::~WgpuAdapterImpl() = default;

Result<scope<Device>> WgpuAdapterImpl::CreateDevice(const DeviceDesc& desc) {
    if (!adapter_) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Adapter is invalid");
    }

#ifdef __EMSCRIPTEN__
    return RequestDevice(desc);
#else
    auto storage = detail::BuildDeviceDescriptor(desc);
    WGPUDevice device = wgpuAdapterCreateDevice(adapter_.get(), &storage.native_desc);
    if (device == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed,
            "Failed to create device '" + desc.label + "'");
    }

    auto device_scope = CreateDeviceFromNative(*this, device, std::move(storage));
    if (!device_scope) {
        wgpuDeviceRelease(device);
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to wrap created device");
    }

    return Ok(std::move(device_scope));
#endif
}

Result<scope<Device>> WgpuAdapterImpl::RequestDevice(const DeviceDesc& desc) {
    if (!adapter_) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Adapter is invalid");
    }

    struct State {
        bool done{false};
        WGPUDevice device{nullptr};
        std::string message{};
    } state;

    auto storage = detail::BuildDeviceDescriptor(desc);

    WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
#ifdef __EMSCRIPTEN__
    callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
#else
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
#endif
    callback_info.callback = [](const WGPURequestDeviceStatus status,
                                WGPUDevice device,
                                const WGPUStringView message,
                                void*,
                                void* userdata) {
        auto* state_ptr = static_cast<State*>(userdata);
        state_ptr->done = true;
        if (status == WGPURequestDeviceStatus_Success) {
            state_ptr->device = device;
        }
        state_ptr->message = detail::StringFromView(message);
    };
    callback_info.userdata2 = &state;

    const auto future = wgpuAdapterRequestDevice(adapter_.get(), &storage.native_desc, callback_info);
    if (future.id == 0) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "WebGPU device request failed to start");
    }

    while (!state.done) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#else
        instance_->ProcessEvents();
#endif
    }

    if (state.device == nullptr) {
        return Err(ErrorCode::GraphicsResourceCreationFailed,
            state.message.empty() ? "Failed to request WebGPU device" : state.message);
    }

    auto device_scope = CreateDeviceFromNative(*this, state.device, std::move(storage));
    if (!device_scope) {
        wgpuDeviceRelease(state.device);
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Failed to wrap requested device");
    }

    return Ok(std::move(device_scope));
}

Future WgpuAdapterImpl::RequestDevice(
    const DeviceDesc& desc,
    CallbackMode callback_mode,
    RequestDeviceCallback callback) {
    Future future{};
    if (!adapter_) {
        future.message = "Adapter is invalid";
        return future;
    }

    if (!callback) {
        future.message = "RequestDevice requires a callback";
        return future;
    }

    auto storage = std::make_shared<detail::DeviceDescriptorStorage>(detail::BuildDeviceDescriptor(desc));
    detail::retain(instance_handle_.get());
    detail::retain(adapter_.get());

    auto* callback_state = new RequestDeviceCallbackState{
        .callback = [callback = std::move(callback),
                        adapter = this,
                        storage,
                        retained_instance = instance_handle_.get(),
                        retained_adapter = adapter_.get()](
                        const WGPURequestDeviceStatus status,
                        WGPUDevice device,
                        const std::string_view message) mutable {
            scope<Device> device_scope{};
            if (status == WGPURequestDeviceStatus_Success && device != nullptr) {
                device_scope = CreateDeviceFromNative(*adapter, device, *storage);
                if (!device_scope) {
                    wgpuDeviceRelease(device);
                }
            } else if (device != nullptr) {
                wgpuDeviceRelease(device);
            }

            callback(FromWgpu(status), std::move(device_scope), message);

            if (retained_adapter != nullptr) {
                wgpuAdapterRelease(retained_adapter);
            }
            if (retained_instance != nullptr) {
                wgpuInstanceRelease(retained_instance);
            }
        },
    };

    WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callback_info.mode = ToWgpu(callback_mode);
    callback_info.callback = RequestDeviceThunk;
    callback_info.userdata1 = callback_state;

    const auto native_future =
        wgpuAdapterRequestDevice(adapter_.get(), &storage->native_desc, callback_info);
    future.id = native_future.id;

    if (future.id == 0) {
        delete callback_state;
        wgpuAdapterRelease(adapter_.get());
        wgpuInstanceRelease(instance_handle_.get());
        future.message = "WebGPU device request failed to start";
    }

    return future;
}

Instance& WgpuAdapterImpl::GetInstance() const noexcept {
    WOKI_ASSERT(instance_ != nullptr);
    return *instance_;
}

AdapterInfo WgpuAdapterImpl::GetInfo() const {
    return detail::QueryAdapterInfo(adapter_.get());
}

Result<void> WgpuAdapterImpl::GetInfo(AdapterInfo& info) const {
    info = detail::QueryAdapterInfo(adapter_.get());
    return Ok();
}

void WgpuAdapterImpl::GetFeatures(SupportedFeatures& features) const {
    features = detail::QuerySupportedFeatures(adapter_.get());
}

SupportedFeatures WgpuAdapterImpl::GetFeatures() const {
    return detail::QuerySupportedFeatures(adapter_.get());
}

Result<void> WgpuAdapterImpl::GetFormatCapabilities(
    const TextureFormat format, DawnFormatCapabilities& capabilities) const {
    if (!adapter_) {
        return Err(ErrorCode::GraphicsResourceCreationFailed, "Adapter is invalid");
    }

    WGPUDawnFormatCapabilities native_capabilities = WGPU_DAWN_FORMAT_CAPABILITIES_INIT;
    native_capabilities.nextInChain = static_cast<WGPUChainedStruct*>(capabilities.next_in_chain);

    if (wgpuAdapterGetFormatCapabilities(adapter_.get(), ToWgpu(format), &native_capabilities)
        != WGPUStatus_Success) {
        return Err(ErrorCode::GraphicsInvalidFormat, "Failed to query format capabilities");
    }

    capabilities.next_in_chain = native_capabilities.nextInChain;
    return Ok();
}

Limits WgpuAdapterImpl::GetLimits() const {
    return detail::QueryLimits(adapter_.get());
}

Result<void> WgpuAdapterImpl::GetLimits(Limits& limits) const {
    return detail::FillLimits(adapter_.get(), limits);
}

bool WgpuAdapterImpl::HasFeature(const FeatureName feature) const noexcept {
    return detail::AdapterHasFeature(adapter_.get(), feature);
}

NativeHandles WgpuAdapterImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = instance_handle_.get();
    handles.adapter = adapter_.get();
    return handles;
}

WGPUAdapter WgpuAdapterImpl::GetNativeAdapter() const noexcept {
    return adapter_.get();
}

WGPUInstance WgpuAdapterImpl::GetNativeInstance() const noexcept {
    return instance_handle_.get();
}

} // namespace woki::rhi::wgpu
