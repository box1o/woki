#include <catch2/catch_test_macros.hpp>

#include <woki/gfx/shaders.hpp>

TEST_CASE("Shader sources distinguish inline and file-backed source") {
    woki::gfx::ShaderSource inline_source{};
    inline_source.source = "@vertex fn main() {}";

    woki::gfx::ShaderSource file_source{};
    file_source.source_path = "shaders/standard.wgsl";

    REQUIRE(inline_source.HasInlineSource());
    REQUIRE_FALSE(inline_source.HasFileSource());
    REQUIRE(file_source.HasFileSource());
    REQUIRE_FALSE(file_source.HasInlineSource());
}

TEST_CASE("Shader stages have stable diagnostic names") {
    REQUIRE(woki::gfx::ToString(woki::gfx::ShaderStage::Vertex) == "Vertex");
    REQUIRE(woki::gfx::ToString(woki::gfx::ShaderStage::Fragment) == "Fragment");
    REQUIRE(woki::gfx::ToString(woki::gfx::ShaderStage::Compute) == "Compute");
}

TEST_CASE("Shader descriptions require unique valid stages") {
    woki::gfx::ShaderDesc desc{};
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());

    desc.sources.push_back({
        .stage = woki::gfx::ShaderStage::Vertex,
        .source = "@vertex fn main() {}",
    });
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.sources.push_back({
        .stage = woki::gfx::ShaderStage::Vertex,
        .source = "@vertex fn other() {}",
    });
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Compute shader stages cannot be mixed with graphics stages") {
    woki::gfx::ShaderDesc desc{};
    desc.sources.push_back({
        .stage = woki::gfx::ShaderStage::Compute,
        .source = "@compute @workgroup_size(1) fn main() {}",
    });
    REQUIRE(woki::gfx::Validate(desc).has_value());

    desc.sources.push_back({
        .stage = woki::gfx::ShaderStage::Fragment,
        .source = "@fragment fn main() {}",
    });
    REQUIRE_FALSE(woki::gfx::Validate(desc).has_value());
}

TEST_CASE("Shader descriptions reject unsupported source languages") {
    woki::gfx::ShaderDesc desc{};
    desc.sources.push_back({
        .stage = woki::gfx::ShaderStage::Vertex,
        .language = woki::gfx::ShaderLanguage::SpirV,
        .source = "binary",
    });

    const auto result = woki::gfx::Validate(desc);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().Code() == woki::ErrorCode::GraphicsUnsupportedApi);
}
