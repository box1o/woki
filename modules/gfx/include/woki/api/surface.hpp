#pragma once

#include "woki/api/descriptors.hpp"

#include <woki/core.hpp>

namespace woki::api {

class Adapter;

class Surface {
public:
    virtual ~Surface() = default;

    [[nodiscard]] virtual SurfaceCapabilities GetCapabilities(const Adapter& adapter) const = 0;
    [[nodiscard]] virtual SurfaceTexture GetCurrentTexture() = 0;
    [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;
    virtual void Configure(const SurfaceConfiguration& config) = 0;
    virtual void Unconfigure() = 0;
    [[nodiscard]] virtual Status Present() = 0;
    virtual void SetLabel(std::string_view label) = 0;

protected:
    Surface() = default;
};

} // namespace woki::api
