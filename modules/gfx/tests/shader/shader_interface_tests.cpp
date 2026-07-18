#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/shaders.hpp>

TEST_CASE("Shader interfaces require unique names and binding slots") {
    woki::gfx::ShaderInterfaceDesc interface{
        .parameters = {{.name = woki::StringId{"roughness"}}},
        .resources = {{.name = woki::StringId{"albedo"}, .group = 1, .binding = 1}},
    };
    REQUIRE(woki::gfx::Validate(interface));

    interface.resources.push_back({.name = woki::StringId{"normal"}, .group = 1, .binding = 1});
    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader interface names are shared across values and resources") {
    const woki::StringId duplicate{"shared"};
    woki::gfx::ShaderInterfaceDesc interface{
        .parameters = {{.name = duplicate}},
        .resources = {{.name = duplicate}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader resources cannot overlap the material parameter block") {
    woki::gfx::ShaderInterfaceDesc interface{
        .parameter_group = 2,
        .parameter_binding = 4,
        .parameters = {{.name = woki::StringId{"color"}}},
        .resources = {{.name = woki::StringId{"texture"}, .group = 2, .binding = 4}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}
