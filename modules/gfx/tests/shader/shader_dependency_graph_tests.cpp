#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <woki/gfx/shaders.hpp>

TEST_CASE("Shader dependency graph finds shaders using a shared include") {
    woki::gfx::ShaderDependencyGraph graph;
    const auto pbr = woki::gfx::ShaderHandle::FromParts(1, 1);
    const auto unlit = woki::gfx::ShaderHandle::FromParts(2, 1);
    const std::vector<std::string> pbr_dependencies{"common.wgsl", "lighting.wgsl"};
    const std::vector<std::string> unlit_dependencies{"common.wgsl"};

    graph.Update(pbr, "pbr.wgsl", pbr_dependencies);
    graph.Update(unlit, "unlit.wgsl", unlit_dependencies);

    REQUIRE(graph.AffectedBy("common.wgsl") == std::vector{pbr, unlit});
    REQUIRE(graph.AffectedBy("lighting.wgsl") == std::vector{pbr});
    REQUIRE(graph.AffectedBy("unrelated.wgsl").empty());
}

TEST_CASE("Shader dependency graph tracks changes to root shader files") {
    woki::gfx::ShaderDependencyGraph graph;
    const auto shader = woki::gfx::ShaderHandle::FromParts(8, 3);

    graph.Update(shader, "materials/custom.wgsl", {});

    REQUIRE(graph.AffectedBy("materials/custom.wgsl") == std::vector{shader});
}

TEST_CASE("Updating shader dependencies removes obsolete reverse edges") {
    woki::gfx::ShaderDependencyGraph graph;
    const auto shader = woki::gfx::ShaderHandle::FromParts(4, 1);
    const std::vector<std::string> initial{"old.wgsl", "shared.wgsl", "shared.wgsl"};
    const std::vector<std::string> replacement{"new.wgsl"};

    graph.Update(shader, "shader.wgsl", initial);
    graph.Update(shader, "shader.wgsl", replacement);

    REQUIRE(graph.AffectedBy("old.wgsl").empty());
    REQUIRE(graph.AffectedBy("shared.wgsl").empty());
    REQUIRE(graph.AffectedBy("new.wgsl") == std::vector{shader});
    REQUIRE(std::ranges::equal(graph.Dependencies(shader), replacement));
}

TEST_CASE("Shader dependency graph combines changed files without duplicate work") {
    woki::gfx::ShaderDependencyGraph graph;
    const auto first = woki::gfx::ShaderHandle::FromParts(1, 1);
    const auto second = woki::gfx::ShaderHandle::FromParts(2, 1);
    const std::vector<std::string> first_dependencies{"a.wgsl", "shared.wgsl"};
    const std::vector<std::string> second_dependencies{"b.wgsl", "shared.wgsl"};

    graph.Update(first, "first.wgsl", first_dependencies);
    graph.Update(second, "second.wgsl", second_dependencies);

    const std::array<std::string, 2> changes{"a.wgsl", "shared.wgsl"};
    REQUIRE(graph.AffectedBy(changes) == std::vector{first, second});
}

TEST_CASE("Removing a shader clears all dependency edges") {
    woki::gfx::ShaderDependencyGraph graph;
    const auto shader = woki::gfx::ShaderHandle::FromParts(6, 2);
    const std::vector<std::string> dependencies{"shared.wgsl"};
    graph.Update(shader, "shader.wgsl", dependencies);

    REQUIRE(graph.Remove(shader));
    REQUIRE_FALSE(graph.Remove(shader));
    REQUIRE(graph.Empty());
    REQUIRE(graph.AffectedBy("shader.wgsl").empty());
    REQUIRE(graph.AffectedBy("shared.wgsl").empty());
}
