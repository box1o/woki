#pragma once

#include "../../include/woki/api/instance.hpp"

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuInstanceImpl final : public Instance {
public:
    explicit WgpuInstanceImpl(const InstanceDesc& desc);
    WgpuInstanceImpl(WGPUInstance handle, const InstanceDesc& desc = {});
    ~WgpuInstanceImpl() override;

    [[nodiscard]] Backend GetBackend() const noexcept override;
    [[nodiscard]] const InstanceDesc& GetDesc() const noexcept override;
    [[nodiscard]] bool IsValid() const noexcept override;
    [[nodiscard]] bool HasFeature(InstanceFeature feature) const noexcept override;
    [[nodiscard]] const std::vector<InstanceFeature>& GetSupportedFeatures() const noexcept override;
    [[nodiscard]] scope<Surface> CreateSurface(const SurfaceDesc& desc) override;
    [[nodiscard]] scope<Adapter> RequestAdapter(const RequestAdapterDesc& desc = {}) override;
    [[nodiscard]] Future RequestAdapter(
        const RequestAdapterDesc& desc,
        CallbackMode callback_mode,
        RequestAdapterCallback callback) override;
    [[nodiscard]] WaitStatus WaitAny(std::vector<FutureWaitInfo>& futures, u64 timeout_ns) override;
    [[nodiscard]] WaitStatus WaitAny(FutureWaitInfo& future, u64 timeout_ns) override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;
    void ProcessEvents() const noexcept override;

    [[nodiscard]] WGPUInstance GetWgpuHandle() const noexcept;
    [[nodiscard]] std::vector<WGPUWGSLLanguageFeatureName> GetWgslLanguageFeatures() const;
    [[nodiscard]] bool HasWgslLanguageFeature(WGPUWGSLLanguageFeatureName feature) const noexcept;
    [[nodiscard]] WGPUSurface CreateSurfaceNative(const WGPUSurfaceDescriptor* descriptor) const noexcept;
    [[nodiscard]] WGPUFuture RequestAdapterNative(
        const WGPURequestAdapterOptions* options,
        WGPUCallbackMode callback_mode,
        std::function<void(WGPURequestAdapterStatus, WGPUAdapter, std::string_view)> callback) const;
    [[nodiscard]] WGPUWaitStatus WaitAny(
        size_t future_count,
        WGPUFutureWaitInfo* futures,
        u64 timeout_ns) const noexcept;

private:
    bool Initialize() noexcept;

    InstanceDesc desc_;
    std::vector<InstanceFeature> supported_features_;
    WGPUInstance handle_{nullptr};
};

} // namespace woki::api::wgpu
