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
