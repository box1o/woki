#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <woki/gfx/shaders.hpp>

namespace {

class TemporaryShaderFile final {
public:
    TemporaryShaderFile() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("woki_shader_watcher_" + std::to_string(suffix) + ".wgsl");
        Remove();
    }

    ~TemporaryShaderFile() { Remove(); }

    void Write(const std::string_view source) const {
        std::ofstream stream(path_, std::ios::binary | std::ios::trunc);
        stream << source;
    }

    void Remove() const noexcept {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& Path() const noexcept { return path_; }

private:
    std::filesystem::path path_{};
};

} // namespace

TEST_CASE("Shader file watcher reports file creation and removal") {
    TemporaryShaderFile file;
    woki::gfx::ShaderFileWatcher watcher;
    REQUIRE(watcher.Watch(file.Path()).has_value());
    REQUIRE(watcher.Poll().empty());

    file.Write("fn shared() {}\n");
    const auto created = watcher.Poll();
    REQUIRE(created.size() == 1);
    REQUIRE(created.front().type == woki::gfx::ShaderFileChangeType::Created);

    file.Remove();
    const auto removed = watcher.Poll();
    REQUIRE(removed.size() == 1);
    REQUIRE(removed.front().type == woki::gfx::ShaderFileChangeType::Removed);
}

TEST_CASE("Shader file watcher reports content modifications") {
    TemporaryShaderFile file;
    file.Write("a");
    woki::gfx::ShaderFileWatcher watcher;
    REQUIRE(watcher.Watch(file.Path()).has_value());

    file.Write("a larger shader source");
    const auto changes = watcher.Poll();

    REQUIRE(changes.size() == 1);
    REQUIRE(changes.front().type == woki::gfx::ShaderFileChangeType::Modified);
    REQUIRE(watcher.Poll().empty());
}

TEST_CASE("Shader file watcher manages watched paths") {
    TemporaryShaderFile file;
    woki::gfx::ShaderFileWatcher watcher;

    REQUIRE(watcher.Watch(file.Path()).has_value());
    REQUIRE(watcher.IsWatching(file.Path()));
    REQUIRE(watcher.WatchedFileCount() == 1);
    REQUIRE(watcher.Unwatch(file.Path()));
    REQUIRE_FALSE(watcher.Unwatch(file.Path()));
    REQUIRE(watcher.Empty());
}
