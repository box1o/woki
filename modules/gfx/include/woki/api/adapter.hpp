#pragma once

#include "woki/api/descriptors.hpp"

#include <woki/core.hpp>

namespace woki::api {

class Device;
class Instance;

class Adapter {
public:
    virtual ~Adapter() = default;

    [[nodiscard]] virtual scope<Device> CreateDevice(const DeviceDesc& desc = {}) = 0;
    [[nodiscard]] virtual scope<Device> RequestDevice(const DeviceDesc& desc = {}) = 0;
    [[nodiscard]] virtual Future RequestDevice(
        const DeviceDesc& desc,
        CallbackMode callback_mode,
        RequestDeviceCallback callback) = 0;
    [[nodiscard]] virtual Instance* GetInstance() const noexcept = 0;
    [[nodiscard]] virtual AdapterInfo GetInfo() const = 0;
    [[nodiscard]] virtual Status GetInfo(AdapterInfo* info) const = 0;
    [[nodiscard]] virtual Limits GetLimits() const = 0;
    [[nodiscard]] virtual Status GetLimits(Limits* limits) const = 0;
    [[nodiscard]] virtual SupportedFeatures GetFeatures() const = 0;
    [[nodiscard]] virtual bool HasFeature(FeatureName feature) const = 0;
    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;

protected:
    Adapter() = default;
};

} // namespace woki::api
