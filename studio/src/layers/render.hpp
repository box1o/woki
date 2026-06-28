#pragma once

#include "core/layer.hpp"

#include <functional>
#include <optional>

namespace woki {

class RenderLayer final : public Layer {
public:
    void OnAttach(Context& ctx) override;
    void OnDetach(Context& ctx) override;
    void OnUpdate(Context& ctx, f64 delta_ms) override;

private:
    void BeginGraphicsInitialization(Window& window);
    void FinalizeGraphicsInitialization(Window& window);
    [[nodiscard]] bool IsGraphicsReady() const noexcept;

    scope<api::Instance> instance_;
    scope<api::Surface> surface_;
    scope<api::Adapter> adapter_;
    scope<api::Device> device_;
    scope<api::Swapchain> swapchain_;
    std::optional<std::reference_wrapper<Window>> window_;
    CallbackId window_resize_callback_id_{0};
    bool graphics_initialization_started_{false};
    bool graphics_initialization_failed_{false};
};

} // namespace woki
