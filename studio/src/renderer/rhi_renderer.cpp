#include "rhi_renderer.hpp"

#include <woki/gfx.hpp>

namespace woki {
    RhiRenderer::RhiRenderer() {

        woki::gfx::ShaderPreprocessor preprocessor{};
        auto result = preprocessor.Process("assets/shaders/default.wgsl");

        if (!result) {
            slog::Error("Shader preprocessing failed: {}", result.error().Message());
            return;
        }

        slog::Info("Preprocessed shader:\n{}", result->source);
        for (const auto& dependency : result->dependencies) {
            slog::Info("Shader dependency: {}", dependency.generic_string());
        }

    }

    RhiRenderer::~RhiRenderer() {

    }

} // namespace woki
