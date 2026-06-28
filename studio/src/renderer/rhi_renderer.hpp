#pragma once

#include <woki/window/window.hpp>

namespace woki::rhi {
class Adapter;
class Device;
class Instance;
class Surface;
class Swapchain;
class TextureView;
}

namespace woki {

class RhiRenderer {
public:
    RhiRenderer();
    ~RhiRenderer();

    RhiRenderer(const RhiRenderer&) = delete;
    RhiRenderer& operator=(const RhiRenderer&) = delete;

    [[nodiscard]] bool Initialize(Window& window);
    void Shutdown() noexcept;
    void Resize(u32 width, u32 height);
    [[nodiscard]] bool RenderFrame();

    [[nodiscard]] bool IsReady() const noexcept { return ready_; }

private:
    scope<rhi::Instance> instance_;
    scope<rhi::Surface> surface_;
    scope<rhi::Adapter> adapter_;
    scope<rhi::Device> device_;
    scope<rhi::Swapchain> swapchain_;
    Window* window_{nullptr};
    u32 width_{0};
    u32 height_{0};
    bool ready_{false};
};

} // namespace woki
