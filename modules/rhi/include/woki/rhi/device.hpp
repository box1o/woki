#pragma once

#include "types.hpp"

#include <woki/core.hpp>

namespace woki::rhi {

class Adapter;

class Device {
public:
    virtual ~Device() = default;

    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;
    virtual void Destroy() = 0;
    virtual void Tick() const noexcept = 0;
    virtual void SetLabel(std::string_view label) = 0;

protected:
    Device() = default;
};

} // namespace woki::rhi
