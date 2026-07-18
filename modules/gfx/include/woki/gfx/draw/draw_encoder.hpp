#pragma once

#include "draw_state.hpp"

#include <woki/rhi/render_pass_encoder.hpp>

namespace woki::gfx {

class DrawBindingEncoder {
public:
    virtual ~DrawBindingEncoder() = default;

    [[nodiscard]] virtual Result<void> Prepare(const ResolvedDrawList& draws) = 0;
    virtual void Encode(rhi::RenderPassEncoder& pass, const ResolvedDraw& draw, u32 draw_index) = 0;
};

struct DrawEncodingStats final {
    u32 draw_calls{0};
    u32 pipeline_bindings{0};
    u32 geometry_bindings{0};
    u32 binding_updates{0};
};

[[nodiscard]] Result<void> Validate(const ResolvedDrawList& draws);
[[nodiscard]] Result<DrawEncodingStats> EncodeDraws(rhi::RenderPassEncoder& pass,
    const ResolvedDrawList& draws, DrawBindingEncoder* bindings = nullptr);

} // namespace woki::gfx
