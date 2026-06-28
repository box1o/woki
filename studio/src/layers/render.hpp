#pragma once

#include "core/layer.hpp"

#include <woki/gfx.hpp>

namespace woki {

class RenderLayer final : public Layer {
public:
    void OnAttach(Context& ctx) override;
    void OnDetach(Context& ctx) override;
    void OnUpdate(Context& ctx, f64 delta_ms) override;
};

} // namespace woki
