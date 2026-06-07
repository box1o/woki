#pragma once

#include "../../include/woki/api/surface.hpp"

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuSurfaceImpl final : public Surface {
public:
    WgpuSurfaceImpl(WGPUInstance instance, WGPUSurface surface, Window* window = nullptr);
    ~WgpuSurfaceImpl() override;

    [[nodiscard]] SurfaceCapabilities GetCapabilities(const Adapter& adapter) const override;
    [[nodiscard]] SurfaceTexture GetCurrentTexture() override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;
    void Configure(const SurfaceConfiguration& config) override;
    void Unconfigure() override;
    [[nodiscard]] Status Present() override;
    void SetLabel(std::string_view label) override;

    [[nodiscard]] WGPUSurface GetWgpuHandle() const noexcept;
    [[nodiscard]] Window* GetWindow() const noexcept;

private:
    void ReleaseCurrentTexture() noexcept;

    WGPUInstance instance_{nullptr};
    WGPUSurface surface_{nullptr};
    Window* window_{nullptr};
    SurfaceConfiguration current_config_{};
    WGPUTexture current_texture_{nullptr};
    WGPUTextureView current_view_{nullptr};
};

[[nodiscard]] WGPUSurface CreateNativeSurface(WGPUInstance instance, Window* window);

#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
[[nodiscard]] WGPUSurface CreateNativeSurfaceCocoa(WGPUInstance instance, Window* window);
#endif

} // namespace woki::api::wgpu
