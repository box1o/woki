#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/shaders.hpp>

TEST_CASE("Standard shader library describes hot reloadable PBR") {
    const woki::gfx::StandardShaderLibrary library{"renderer/shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::Pbr);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/pbr"});
    REQUIRE(shader.sources.size() == 2);
    REQUIRE(shader.sources[0].source_path == "renderer/shaders/pbr_forward.wgsl");
    REQUIRE(shader.sources[0].entry_point == "vertex_main");
    REQUIRE(shader.sources[1].entry_point == "fragment_main");
    REQUIRE(shader.hot_reload);
    REQUIRE(shader.interface.uses_object_transform);
    REQUIRE(shader.interface.uses_lighting);
    REQUIRE(shader.interface.parameters.size() == 4);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes unlit rendering") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::Unlit);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/unlit"});
    REQUIRE(shader.interface.uses_object_transform);
    REQUIRE_FALSE(shader.interface.uses_lighting);
    REQUIRE(shader.interface.parameters.size() == 1);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes skinned PBR") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::PbrSkinned);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/pbr_skinned"});
    REQUIRE(shader.interface.uses_object_transform);
    REQUIRE(shader.interface.uses_skinning);
    REQUIRE(shader.interface.uses_lighting);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes textured PBR resources") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::PbrTextured);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/pbr_textured"});
    REQUIRE(shader.interface.parameters.size() == 7);
    REQUIRE(shader.interface.resources.size() == 6);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes shadowed PBR frame bindings") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::PbrShadowed);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/pbr_shadowed"});
    REQUIRE(shader.interface.uses_lighting);
    REQUIRE(shader.interface.uses_shadows);
    REQUIRE(shader.interface.shadow_group == shader.interface.lighting_group);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes tone mapping") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::ToneMap);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/tone_map"});
    REQUIRE(shader.sources.front().source_path == "shaders/tone_map.wgsl");
    REQUIRE(shader.interface.resources.size() == 2);
    REQUIRE_FALSE(shader.interface.uses_object_transform);
    REQUIRE(woki::gfx::Validate(shader));
}

TEST_CASE("Standard shader library describes environment PBR frame bindings") {
    const woki::gfx::StandardShaderLibrary library{"shaders"};
    const auto shader = library.Describe(woki::gfx::StandardShader::PbrEnvironment);

    REQUIRE(shader.asset_id == woki::gfx::AssetId{"woki/shaders/pbr_environment"});
    REQUIRE(shader.interface.uses_lighting);
    REQUIRE(shader.interface.uses_environment);
    REQUIRE(shader.interface.environment_group == shader.interface.lighting_group);
    REQUIRE(woki::gfx::Validate(shader));
}
