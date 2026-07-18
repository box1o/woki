#pragma once

#include "shader.hpp"

#include <woki/path/path.hpp>

namespace woki::gfx {

class ShaderManager;

enum class StandardShader : u8 {
    Unlit = 0,
    Pbr,
    PbrShadowed,
    PbrEnvironment,
    PbrTextured,
    PbrSkinned,
    ToneMap,
    DepthMasked,
    PbrFull,
    PbrFullSkinned,
};

class StandardShaderLibrary final {
public:
    explicit StandardShaderLibrary(paths::Path shader_root);

    [[nodiscard]] const paths::Path& Root() const noexcept;
    [[nodiscard]] ShaderDesc Describe(StandardShader shader) const;
    [[nodiscard]] Result<ShaderHandle> Load(StandardShader shader, ShaderManager& manager) const;

private:
    paths::Path root_{};
};

} // namespace woki::gfx
