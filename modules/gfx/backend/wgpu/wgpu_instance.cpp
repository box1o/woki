#include "wgpu_instance.hpp"

#include "common.hpp"
#include "wgpu_adapter.hpp"
#include "wgpu_surface.hpp"

#include <limits>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifndef __EMSCRIPTEN__
#include <dawn/native/DawnNative.h>
#endif

namespace woki::api::wgpu {

namespace {

struct RequestAdapterCallbackState {
    std::function<void(WGPURequestAdapterStatus, WGPUAdapter, std::string_view)> callback;
};

[[nodiscard]] constexpr std::string_view ToString(WGPURequestAdapterStatus status) noexcept {
    switch (status) {
    case WGPURequestAdapterStatus_Success:
        return "Success";
    case WGPURequestAdapterStatus_CallbackCancelled:
        return "CallbackCancelled";
    case WGPURequestAdapterStatus_Unavailable:
        return "Unavailable";
    case WGPURequestAdapterStatus_Error:
        return "Error";
    case WGPURequestAdapterStatus_Force32:
        break;
    }

    return "Unknown";
}

void RequestAdapterThunk(WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView message,
    void* userdata1,
    void*) {
    auto callback_state = scope<RequestAdapterCallbackState>(
        static_cast<RequestAdapterCallbackState*>(userdata1));
    if (callback_state == nullptr || !callback_state->callback) {
        return;
    }

    const auto message_text = detail::StringFromView(message);
    if (status != WGPURequestAdapterStatus_Success) {
        slog::Error("WebGPU adapter request completed with status {}: {}", ToString(status),
            message_text.empty() ? "(no message)" : message_text);
    }

    callback_state->callback(status, adapter, message_text);
}

} // namespace

WgpuInstanceImpl::WgpuInstanceImpl(const InstanceDesc& desc)
    : desc_(desc)
    , supported_features_(detail::QuerySupportedInstanceFeatures()) {
    Initialize();
}

WgpuInstanceImpl::WgpuInstanceImpl(WGPUInstance handle, const InstanceDesc& desc)
    : desc_(desc)
    , supported_features_(detail::QuerySupportedInstanceFeatures())
    , handle_(handle) {
    if (desc_.backend == Backend::kAuto) {
        desc_.backend = Backend::kWebGpu;
    }
}

WgpuInstanceImpl::~WgpuInstanceImpl() {
    if (handle_ != nullptr) {
        wgpuInstanceRelease(handle_);
        handle_ = nullptr;
    }
}

Backend WgpuInstanceImpl::GetBackend() const noexcept { return Backend::kWebGpu; }

const InstanceDesc& WgpuInstanceImpl::GetDesc() const noexcept { return desc_; }

bool WgpuInstanceImpl::IsValid() const noexcept { return handle_ != nullptr; }

bool WgpuInstanceImpl::HasFeature(InstanceFeature feature) const noexcept {
    return std::ranges::find(supported_features_, feature) != supported_features_.end();
}

const std::vector<InstanceFeature>& WgpuInstanceImpl::GetSupportedFeatures() const noexcept {
    return supported_features_;
}

scope<Surface> WgpuInstanceImpl::CreateSurface(const SurfaceDesc& desc) {
    if (desc.window == nullptr) {
        slog::Error("Cannot create a surface without a window");
        return nullptr;
    }

    WGPUSurface surface = CreateNativeSurface(handle_, desc.window);
    if (surface == nullptr) {
        slog::Error("Failed to create WebGPU surface '{}'", desc.label);
        return nullptr;
    }

    wgpuInstanceAddRef(handle_);
    return scope<Surface>(new WgpuSurfaceImpl(handle_, surface, desc.window));
}

scope<Adapter> WgpuInstanceImpl::RequestAdapter(const RequestAdapterDesc& desc) {
    struct State {
        bool done{false};
        WGPUAdapter adapter{nullptr};
        std::string message{};
    } state;

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    options.featureLevel = detail::ToWgpuFeatureLevel(desc.feature_level);
    options.powerPreference = detail::ToWgpuPowerPreference(desc.power_preference);
    options.backendType = detail::ToWgpuBackendType(desc.backend_type);
    options.forceFallbackAdapter = desc.force_fallback_adapter;
    if (desc.compatible_surface != nullptr) {
        const auto* surface = dynamic_cast<const WgpuSurfaceImpl*>(desc.compatible_surface);
        if (surface != nullptr) {
            options.compatibleSurface = surface->GetWgpuHandle();
        }
    }

    WGPURequestAdapterCallbackInfo callback_info = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
#ifdef __EMSCRIPTEN__
    callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
#else
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
#endif
    callback_info.callback = [](WGPURequestAdapterStatus status,
                                WGPUAdapter adapter,
                                WGPUStringView message,
                                void*,
                                void* userdata) {
        auto* state_ptr = static_cast<State*>(userdata);
        state_ptr->done = true;
        if (status == WGPURequestAdapterStatus_Success) {
            state_ptr->adapter = adapter;
        }
        state_ptr->message = detail::StringFromView(message);
    };
    callback_info.userdata2 = &state;

    const auto future = wgpuInstanceRequestAdapter(handle_, &options, callback_info);
    if (future.id == 0) {
        slog::Error("WebGPU adapter request failed to start");
        return nullptr;
    }

    while (!state.done) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#else
        ProcessEvents();
#endif
    }

    if (state.adapter == nullptr) {
        slog::Error("Failed to request WebGPU adapter: {}",
            state.message.empty() ? std::string{"(no message)"} : state.message);
        return nullptr;
    }

    wgpuInstanceAddRef(handle_);
    return scope<Adapter>(new WgpuAdapterImpl(handle_, state.adapter));
}

Future WgpuInstanceImpl::RequestAdapter(
    const RequestAdapterDesc& desc,
    CallbackMode callback_mode,
    RequestAdapterCallback callback) {
    Future future{};
    if (handle_ == nullptr) {
        future.message = "Instance is invalid";
        return future;
    }

    if (!callback) {
        future.message = "RequestAdapter requires a callback";
        return future;
    }

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    options.featureLevel = detail::ToWgpuFeatureLevel(desc.feature_level);
    options.powerPreference = detail::ToWgpuPowerPreference(desc.power_preference);
    options.backendType = detail::ToWgpuBackendType(desc.backend_type);
    options.forceFallbackAdapter = desc.force_fallback_adapter;
    if (desc.compatible_surface != nullptr) {
        const auto* surface = dynamic_cast<const WgpuSurfaceImpl*>(desc.compatible_surface);
        if (surface != nullptr) {
            options.compatibleSurface = surface->GetWgpuHandle();
        }
    }

    wgpuInstanceAddRef(handle_);

    future.id = RequestAdapterNative(&options,
        detail::ToWgpuCallbackMode(callback_mode),
        [callback = std::move(callback), handle = handle_](WGPURequestAdapterStatus status,
            WGPUAdapter adapter,
            std::string_view message) mutable {
            scope<Adapter> adapter_scope{};
            if (status == WGPURequestAdapterStatus_Success && adapter != nullptr) {
                adapter_scope = scope<Adapter>(new WgpuAdapterImpl(handle, adapter));
                handle = nullptr;
            }

            callback(detail::FromWgpuRequestAdapterStatus(status), std::move(adapter_scope), message);

            if (handle != nullptr) {
                wgpuInstanceRelease(handle);
            }
        })
                    .id;

    if (future.id == 0) {
        wgpuInstanceRelease(handle_);
        future.message = "WebGPU adapter request failed to start";
    }

    return future;
}

WaitStatus WgpuInstanceImpl::WaitAny(std::vector<FutureWaitInfo>& futures, u64 timeout_ns) {
    if (futures.empty()) {
        return WaitStatus::kUnsupportedCount;
    }

    std::vector<WGPUFutureWaitInfo> native_futures(futures.size(), WGPU_FUTURE_WAIT_INFO_INIT);
    for (size_t i = 0; i < futures.size(); ++i) {
        native_futures[i].future.id = futures[i].future.id;
        native_futures[i].completed = futures[i].completed;
    }

    const auto status = WaitAny(native_futures.size(), native_futures.data(), timeout_ns);
    for (size_t i = 0; i < futures.size(); ++i) {
        futures[i].completed = native_futures[i].completed;
    }

    return detail::FromWgpuWaitStatus(status);
}

WaitStatus WgpuInstanceImpl::WaitAny(FutureWaitInfo& future, u64 timeout_ns) {
    std::vector<FutureWaitInfo> futures{future};
    const auto status = WaitAny(futures, timeout_ns);
    future = futures.front();
    return status;
}

NativeHandles WgpuInstanceImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = handle_;
    return handles;
}

std::vector<WGPUWGSLLanguageFeatureName> WgpuInstanceImpl::GetWgslLanguageFeatures() const {
    if (handle_ == nullptr) {
        return {};
    }

    WGPUSupportedWGSLLanguageFeatures native_features = WGPU_SUPPORTED_WGSL_LANGUAGE_FEATURES_INIT;
    wgpuInstanceGetWGSLLanguageFeatures(handle_, &native_features);

    std::vector<WGPUWGSLLanguageFeatureName> features;
    features.reserve(native_features.featureCount);
    for (size_t i = 0; i < native_features.featureCount; ++i) {
        features.push_back(native_features.features[i]);
    }

    wgpuSupportedWGSLLanguageFeaturesFreeMembers(native_features);
    return features;
}

bool WgpuInstanceImpl::HasWgslLanguageFeature(WGPUWGSLLanguageFeatureName feature) const noexcept {
    return handle_ != nullptr && wgpuInstanceHasWGSLLanguageFeature(handle_, feature);
}

WGPUInstance WgpuInstanceImpl::GetWgpuHandle() const noexcept { return handle_; }

WGPUSurface WgpuInstanceImpl::CreateSurfaceNative(const WGPUSurfaceDescriptor* descriptor) const noexcept {
    if (handle_ == nullptr || descriptor == nullptr) {
        return nullptr;
    }

    return wgpuInstanceCreateSurface(handle_, descriptor);
}

WGPUFuture WgpuInstanceImpl::RequestAdapterNative(
    const WGPURequestAdapterOptions* options,
    WGPUCallbackMode callback_mode,
    std::function<void(WGPURequestAdapterStatus, WGPUAdapter, std::string_view)> callback) const {
    if (handle_ == nullptr) {
        slog::Error("Cannot request a WebGPU adapter from an invalid instance");
        return WGPU_FUTURE_INIT;
    }

    if (!callback) {
        slog::Error("RequestAdapter requires a callback");
        return WGPU_FUTURE_INIT;
    }

    auto* callback_state = new RequestAdapterCallbackState{.callback = std::move(callback)};

    WGPURequestAdapterCallbackInfo callback_info = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    callback_info.mode = callback_mode;
    callback_info.callback = RequestAdapterThunk;
    callback_info.userdata1 = callback_state;

    const auto future = wgpuInstanceRequestAdapter(handle_, options, callback_info);
    if (future.id == 0) {
        delete callback_state;
        slog::Error("WebGPU adapter request failed to start");
    }

    return future;
}

void WgpuInstanceImpl::ProcessEvents() const noexcept {
    if (handle_ == nullptr) {
        return;
    }

    wgpuInstanceProcessEvents(handle_);
}

WGPUWaitStatus WgpuInstanceImpl::WaitAny(
    size_t future_count,
    WGPUFutureWaitInfo* futures,
    u64 timeout_ns) const noexcept {
    if (handle_ == nullptr) {
        return WGPUWaitStatus_Error;
    }

    if (future_count == 0 || futures == nullptr) {
        return WGPUWaitStatus_Error;
    }

    return wgpuInstanceWaitAny(handle_, future_count, futures, timeout_ns);
}

bool WgpuInstanceImpl::Initialize() noexcept {
    const auto native_features = detail::BuildRequiredInstanceFeatures(desc_.required_features);
    if (desc_.required_features.size() != native_features.size()) {
        slog::Error("Failed to create WebGPU instance '{}': unsupported required instance feature",
            desc_.label);
        return false;
    }

    WGPUInstanceLimits native_limits = WGPU_INSTANCE_LIMITS_INIT;
    if (desc_.required_limits.has_value()) {
        WOKI_ASSERT(desc_.required_limits->timed_wait_any_max_count <=
            static_cast<u64>(std::numeric_limits<size_t>::max()));
        native_limits.timedWaitAnyMaxCount =
            static_cast<size_t>(desc_.required_limits->timed_wait_any_max_count);
    }

    WGPUInstanceDescriptor native_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    native_desc.requiredFeatureCount = native_features.size();
    native_desc.requiredFeatures = native_features.empty() ? nullptr : native_features.data();
    native_desc.requiredLimits = desc_.required_limits.has_value() ? &native_limits : nullptr;

#ifndef __EMSCRIPTEN__
    dawn::native::DawnInstanceDescriptor dawn_desc;
    dawn_desc.backendValidationLevel = desc_.enable_validation
        ? dawn::native::BackendValidationLevel::Full
        : dawn::native::BackendValidationLevel::Disabled;
    native_desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&dawn_desc);
#endif

    handle_ = wgpuCreateInstance(&native_desc);
    if (handle_ == nullptr) {
        slog::Error("Failed to create WebGPU instance '{}'", desc_.label);
        return false;
    }

    slog::Info("Created WebGPU instance '{}'", desc_.label);
    return true;
}

} // namespace woki::api::wgpu
