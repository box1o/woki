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

TEST_CASE("Shader lighting cannot overlap material bindings") {
    woki::gfx::ShaderInterfaceDesc interface{
        .uses_lighting = true,
        .lighting_group = 1,
        .lighting_binding = 2,
        .resources = {{.name = woki::StringId{"albedo"}, .group = 1, .binding = 2}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader object transforms cannot overlap material bindings") {
    const woki::gfx::ShaderInterfaceDesc interface{
        .uses_object_transform = true,
        .object_group = 1,
        .object_binding = 0,
        .parameter_group = 1,
        .parameter_binding = 0,
        .parameters = {{.name = woki::StringId{"roughness"}}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader object transforms use a dedicated bind group") {
    const woki::gfx::ShaderInterfaceDesc interface{
        .uses_object_transform = true,
        .object_group = 1,
        .object_binding = 0,
        .parameter_group = 1,
        .parameter_binding = 1,
        .parameters = {{.name = woki::StringId{"roughness"}}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader frame data uses a dedicated bind group") {
    const woki::gfx::ShaderInterfaceDesc interface{
        .parameter_group = 2,
        .uses_lighting = true,
        .lighting_group = 2,
        .parameters = {{.name = woki::StringId{"roughness"}}},
    };

    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader shadows require lighting frame bindings") {
    const woki::gfx::ShaderInterfaceDesc interface{.uses_shadows = true};
    REQUIRE_FALSE(woki::gfx::Validate(interface));
}

TEST_CASE("Shader environments require lighting frame bindings") {
    const woki::gfx::ShaderInterfaceDesc interface{.uses_environment = true};
    REQUIRE_FALSE(woki::gfx::Validate(interface));
}
