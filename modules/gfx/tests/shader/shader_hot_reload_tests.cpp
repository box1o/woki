#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <woki/gfx/shaders.hpp>

namespace {

class ShaderWorkspace final {
public:
    ShaderWorkspace() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path() /
                     ("woki_shader_reload_" + std::to_string(suffix));
        std::filesystem::create_directories(directory_);
    }

    ~ShaderWorkspace() {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    [[nodiscard]] std::filesystem::path File(const std::string_view name) const {
        return directory_ / name;
    }

    void Write(const std::string_view name, const std::string_view source) const {
        std::ofstream stream(File(name), std::ios::binary | std::ios::trunc);
        stream << source;
    }

private:
    std::filesystem::path directory_{};
};

} // namespace

TEST_CASE("Shader hot reload batches shaders affected by shared file changes") {
    ShaderWorkspace workspace;
    workspace.Write("pbr.wgsl", "pbr");
    workspace.Write("unlit.wgsl", "unlit");
    workspace.Write("common.wgsl", "common");

    woki::gfx::ShaderHotReload reload;
    const auto pbr = woki::gfx::ShaderHandle::FromParts(1, 1);
    const auto unlit = woki::gfx::ShaderHandle::FromParts(2, 1);
    const std::vector<std::string> shared{workspace.File("common.wgsl").generic_string()};
    REQUIRE(reload.Track(pbr, workspace.File("pbr.wgsl"), shared).has_value());
    REQUIRE(reload.Track(unlit, workspace.File("unlit.wgsl"), shared).has_value());
    REQUIRE(reload.Poll().Empty());

    workspace.Write("common.wgsl", "a larger shared shader library");
    const auto batch = reload.Poll();

    REQUIRE(batch.changes.size() == 1);
    REQUIRE(batch.shaders == std::vector{pbr, unlit});
    REQUIRE(reload.Poll().Empty());
}

TEST_CASE("Shader hot reload reference counts files shared by shaders") {
    ShaderWorkspace workspace;
    workspace.Write("first.wgsl", "first");
    workspace.Write("second.wgsl", "second");
    workspace.Write("shared.wgsl", "shared");

    woki::gfx::ShaderHotReload reload;
    const auto first = woki::gfx::ShaderHandle::FromParts(4, 1);
    const auto second = woki::gfx::ShaderHandle::FromParts(5, 1);
    const std::vector<std::string> shared{workspace.File("shared.wgsl").generic_string()};
    REQUIRE(reload.Track(first, workspace.File("first.wgsl"), shared).has_value());
    REQUIRE(reload.Track(second, workspace.File("second.wgsl"), shared).has_value());
    REQUIRE(reload.WatchedFileCount() == 3);

    REQUIRE(reload.Untrack(first));
    REQUIRE(reload.WatchedFileCount() == 2);
    REQUIRE(reload.Untrack(second));
    REQUIRE(reload.WatchedFileCount() == 0);
    REQUIRE(reload.ShaderCount() == 0);
}

TEST_CASE("Updating a tracked shader replaces its watched dependency set") {
    ShaderWorkspace workspace;
    workspace.Write("shader.wgsl", "shader");
    workspace.Write("old.wgsl", "old");
    workspace.Write("new.wgsl", "new");

    woki::gfx::ShaderHotReload reload;
    const auto shader = woki::gfx::ShaderHandle::FromParts(7, 1);
    const std::vector<std::string> old_dependency{workspace.File("old.wgsl").generic_string()};
    const std::vector<std::string> new_dependency{workspace.File("new.wgsl").generic_string()};
    REQUIRE(reload.Track(shader, workspace.File("shader.wgsl"), old_dependency).has_value());
    REQUIRE(reload.Track(shader, workspace.File("shader.wgsl"), new_dependency).has_value());
    REQUIRE(reload.WatchedFileCount() == 2);

    workspace.Write("old.wgsl", "old changed but no longer used");
    REQUIRE(reload.Poll().Empty());

    workspace.Write("new.wgsl", "new dependency changed");
    REQUIRE(reload.Poll().shaders == std::vector{shader});
}

TEST_CASE("Shader hot reload validates tracking requests") {
    woki::gfx::ShaderHotReload reload;

    REQUIRE_FALSE(reload.Track({}, "shader.wgsl").has_value());
    REQUIRE_FALSE(reload.Track(woki::gfx::ShaderHandle::FromParts(1, 1), {}).has_value());
}
