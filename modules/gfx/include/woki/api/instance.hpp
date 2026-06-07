#pragma once

#include "woki/api/adapter.hpp"
#include "woki/api/descriptors.hpp"
#include "woki/api/surface.hpp"

#include <vector>

#include <woki/core.hpp>

namespace woki::api {

class Instance {
public:
    virtual ~Instance() = default;

    [[nodiscard]] static scope<Instance> Create(const InstanceDesc& desc = {});

    [[nodiscard]] virtual Backend GetBackend() const noexcept = 0;
    [[nodiscard]] virtual const InstanceDesc& GetDesc() const noexcept = 0;
    [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    [[nodiscard]] virtual bool HasFeature(InstanceFeature feature) const noexcept = 0;
    [[nodiscard]] virtual const std::vector<InstanceFeature>& GetSupportedFeatures() const noexcept = 0;
    [[nodiscard]] virtual scope<Surface> CreateSurface(const SurfaceDesc& desc) = 0;
    [[nodiscard]] virtual scope<Adapter> RequestAdapter(const RequestAdapterDesc& desc = {}) = 0;
    [[nodiscard]] virtual Future RequestAdapter(
        const RequestAdapterDesc& desc,
        CallbackMode callback_mode,
        RequestAdapterCallback callback) = 0;
    [[nodiscard]] virtual WaitStatus WaitAny(std::vector<FutureWaitInfo>& futures, u64 timeout_ns) = 0;
    [[nodiscard]] virtual WaitStatus WaitAny(FutureWaitInfo& future, u64 timeout_ns) = 0;
    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;
    virtual void ProcessEvents() const noexcept = 0;

protected:
    Instance() = default;
};

} // namespace woki::api
