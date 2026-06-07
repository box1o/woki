#pragma once

#include "woki/api/descriptors.hpp"

#include <woki/core.hpp>

namespace woki::api {

class Adapter;
class Swapchain;

class Device {
public:
    virtual ~Device() = default;

    [[nodiscard]] virtual scope<Adapter> GetAdapter() const = 0;
    [[nodiscard]] virtual AdapterInfo GetAdapterInfo() const = 0;
    [[nodiscard]] virtual Status GetAdapterInfo(AdapterInfo* adapter_info) const = 0;
    [[nodiscard]] virtual Limits GetLimits() const = 0;
    [[nodiscard]] virtual Status GetLimits(Limits* limits) const = 0;
    [[nodiscard]] virtual SupportedFeatures GetFeatures() const = 0;
    [[nodiscard]] virtual bool HasFeature(FeatureName feature) const = 0;
    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;
    [[nodiscard]] virtual void* GetQueueHandle() const noexcept = 0;
    [[nodiscard]] virtual scope<Swapchain> CreateSwapchain(const SwapchainDesc& desc) = 0;
    virtual void Tick() const noexcept = 0;
    virtual void SetLabel(std::string_view label) = 0;

protected:
    Device() = default;
};

} // namespace woki::api
