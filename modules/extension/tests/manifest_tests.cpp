#include <catch2/catch_test_macros.hpp>

#include <woki/ext/ext.hpp>

#include <filesystem>
#include <fstream>
#include <string_view>

namespace {

namespace fs = std::filesystem;

[[nodiscard]] fs::path MakeTempDir(std::string_view name) {
    const fs::path root = fs::temp_directory_path() / "woki_extension_tests" / name;
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

void WriteFile(const fs::path& path, std::string_view contents) {
    std::ofstream output(path);
    REQUIRE(output.good());
    output << contents;
}

constexpr std::string_view kValidManifest = R"(
id: woki.hello
name: Hello
version: 0.1.0
apiVersion: 1
runtime:
  wasm: extension.wasm
permissions:
  - log
)";

} // namespace

TEST_CASE("Extension manifest loads a valid manifest") {
    const fs::path root = MakeTempDir("valid_manifest");
    const fs::path path = root / "manifest.yaml";
    WriteFile(path, kValidManifest);

    auto manifest = woki::ext::LoadManifest(path);
    REQUIRE(manifest.has_value());
    REQUIRE(manifest->id == "woki.hello");
    REQUIRE(manifest->name == "Hello");
    REQUIRE(manifest->version == "0.1.0");
    REQUIRE(manifest->api_version == woki::ext::kApiVersion);
    REQUIRE(manifest->wasm_path == "extension.wasm");
    REQUIRE(woki::ext::HasPermission(*manifest, woki::ext::Permission::Log));
}

TEST_CASE("Extension manifest rejects invalid id") {
    woki::ext::Manifest manifest;
    manifest.id = "Woki.Hello";
    manifest.name = "Hello";
    manifest.version = "0.1.0";
    manifest.permissions = {woki::ext::Permission::Log};

    auto valid = woki::ext::ValidateManifest(manifest);
    REQUIRE_FALSE(valid.has_value());
    REQUIRE(valid.error().Code() == woki::ErrorCode::ValidationInvalidState);
    REQUIRE(valid.error().Message().contains("id"));
    REQUIRE(valid.error().Message().contains("woki.hello"));
}

TEST_CASE("Extension manifest rejects malformed id segments") {
    woki::ext::Manifest manifest;
    manifest.id = "woki.-hello";
    manifest.name = "Hello";
    manifest.version = "0.1.0";
    manifest.permissions = {woki::ext::Permission::Log};

    auto valid = woki::ext::ValidateManifest(manifest);
    REQUIRE_FALSE(valid.has_value());
    REQUIRE(valid.error().Code() == woki::ErrorCode::ValidationInvalidState);
}

TEST_CASE("Extension manifest rejects path traversal") {
    woki::ext::Manifest manifest;
    manifest.id = "woki.hello";
    manifest.name = "Hello";
    manifest.version = "0.1.0";
    manifest.wasm_path = "../extension.wasm";
    manifest.permissions = {woki::ext::Permission::Log};

    auto valid = woki::ext::ValidateManifest(manifest);
    REQUIRE_FALSE(valid.has_value());
    REQUIRE(valid.error().Code() == woki::ErrorCode::ValidationInvalidState);
}

TEST_CASE("Extension manifest rejects unknown permission from YAML") {
    const fs::path root = MakeTempDir("unknown_permission");
    const fs::path path = root / "manifest.yaml";
    WriteFile(path, R"(
id: woki.hello
name: Hello
version: 0.1.0
apiVersion: 1
runtime:
  wasm: extension.wasm
permissions:
  - network
)");

    auto manifest = woki::ext::LoadManifest(path);
    REQUIRE_FALSE(manifest.has_value());
    REQUIRE(manifest.error().Code() == woki::ErrorCode::ValidationInvalidState);
    REQUIRE(manifest.error().Message().contains("network"));
    REQUIRE(manifest.error().Message().contains("log"));
}

TEST_CASE("Extension manifest validates package directory name") {
    woki::ext::Manifest manifest;
    manifest.id = "woki.hello";
    manifest.name = "Hello";
    manifest.version = "0.1.0";
    manifest.permissions = {woki::ext::Permission::Log};

    auto valid = woki::ext::ValidateManifestForPackage(manifest, "woki.other");
    REQUIRE_FALSE(valid.has_value());
    REQUIRE(valid.error().Code() == woki::ErrorCode::ValidationInvalidState);
    REQUIRE(valid.error().Message().contains("woki.other"));
    REQUIRE(valid.error().Message().contains("woki.hello"));
}

TEST_CASE("Extension manifest explains missing fields") {
    const fs::path root = MakeTempDir("missing_fields");
    const fs::path path = root / "manifest.yaml";
    WriteFile(path, R"(
id: woki.hello
name: Hello
version: 0.1.0
apiVersion: 1
runtime:
  wasm: extension.wasm
)");

    auto manifest = woki::ext::LoadManifest(path);
    REQUIRE_FALSE(manifest.has_value());
    REQUIRE(manifest.error().Code() == woki::ErrorCode::ParseMissingField);
    REQUIRE(manifest.error().Message().contains("permissions"));
    REQUIRE(manifest.error().Message().contains("- log"));
}
