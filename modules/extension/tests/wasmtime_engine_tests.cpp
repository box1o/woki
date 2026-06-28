#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <woki/ext/ext.hpp>

#if defined(WOKI_EXTENSION_WITH_WASMTIME)

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;

[[nodiscard]] fs::path MakeTempDir(std::string_view name) {
    const fs::path root = fs::temp_directory_path() / "woki_extension_wasmtime_tests" / name;
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

void WriteFile(const fs::path& path, std::string_view contents) {
    std::ofstream output(path);
    REQUIRE(output.good());
    output << contents;
}

[[nodiscard]] bool CompileWasm(
    const fs::path& source, const fs::path& wasm, std::string_view exports) {
    const std::string command = "clang --target=wasm32-unknown-unknown -nostdlib "
                                "-Wl,--no-entry -Wl,--export-memory " +
                                std::string(exports) + " -o " + wasm.string() + " " +
                                source.string();
    return std::system(command.c_str()) == 0;
}

[[nodiscard]] std::string StandardExports() {
    return "-Wl,--export=ext_api_version "
           "-Wl,--export=ext_init "
           "-Wl,--export=ext_on_tick "
           "-Wl,--export=ext_on_event "
           "-Wl,--export=ext_on_unload";
}

[[nodiscard]] woki::ext::Record MakeRecord(const fs::path& root,
    std::vector<woki::ext::Permission> permissions = {woki::ext::Permission::Log}) {
    woki::ext::Record record;
    record.id = "woki.test";
    record.state = woki::ext::State::PermissionChecked;
    record.manifest.id = "woki.test";
    record.manifest.name = "Test";
    record.manifest.version = "0.1.0";
    record.manifest.api_version = woki::ext::kApiVersion;
    record.manifest.permissions = std::move(permissions);
    record.package.install_root = root;
    record.package.manifest = root / "manifest.yaml";
    record.package.wasm = root / "extension.wasm";
    record.package.data_root = root / "data";
    record.package.cache_root = root / "cache";
    WriteFile(record.package.manifest, "id: woki.test\n");
    return record;
}

[[nodiscard]] woki::ext::wasm::Backend MakeBackend() {
    return woki::ext::wasm::Backend(woki::createScope<woki::ext::wasm::WasmtimeEngine>());
}

} // namespace

TEST_CASE("Wasmtime engine loads initializes ticks and unloads a minimal module") {
    const fs::path root = MakeTempDir("valid");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
char memory_anchor[8192];
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return 0; }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();

    auto loaded = backend.Load(record);
    if (!loaded.has_value()) {
        UNSCOPED_INFO(std::string(loaded.error().Message()));
    }
    REQUIRE(loaded.has_value());
    REQUIRE(record.tier == woki::ext::RuntimeTier::Wasm);
    REQUIRE(backend.Initialize(record).has_value());
    backend.Tick(record, 16.0);
    backend.Unload(record);
}

TEST_CASE("Wasmtime engine reports missing required exports") {
    const fs::path root = MakeTempDir("missing_init");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
char memory_anchor[8192];
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm",
        "-Wl,--export=ext_api_version "
        "-Wl,--export=ext_on_tick "
        "-Wl,--export=ext_on_event "
        "-Wl,--export=ext_on_unload"));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();
    auto loaded = backend.Load(record);
    REQUIRE_FALSE(loaded.has_value());
    REQUIRE(loaded.error().Message().contains("ext_init"));
}

TEST_CASE("Wasmtime engine rejects api version mismatch") {
    const fs::path root = MakeTempDir("api_mismatch");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
char memory_anchor[8192];
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 999; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return 0; }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();
    auto loaded = backend.Load(record);
    REQUIRE_FALSE(loaded.has_value());
    REQUIRE(loaded.error().Message().contains("apiVersion mismatch"));
}

TEST_CASE("Wasmtime engine reports ext_init non-zero status") {
    const fs::path root = MakeTempDir("init_status");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
char memory_anchor[8192];
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return -7; }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();
    REQUIRE(backend.Load(record).has_value());
    auto initialized = backend.Initialize(record);
    REQUIRE_FALSE(initialized.has_value());
    REQUIRE(initialized.error().Message().contains("ext_init"));
}

TEST_CASE("Wasmtime engine permission-gates denied host imports") {
    const fs::path root = MakeTempDir("denied_log_import");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
__attribute__((import_module("woki_host"), import_name("host_log")))
extern int host_log(unsigned level, const char* message, unsigned len);
char memory_anchor[8192];
static const char message[] = "hello";
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return host_log(1, message, 5); }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root, {});
    auto backend = MakeBackend();
    auto loaded = backend.Load(record);
    REQUIRE_FALSE(loaded.has_value());
    REQUIRE(loaded.error().Message().contains("host_log"));
}

TEST_CASE("Wasmtime engine converts invalid guest pointers into init failure") {
    const fs::path root = MakeTempDir("invalid_pointer");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
__attribute__((import_module("woki_host"), import_name("host_log")))
extern int host_log(unsigned level, const char* message, unsigned len);
char memory_anchor[8192];
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return host_log(1, (const char*)0x7fffffff, 4); }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();
    REQUIRE(backend.Load(record).has_value());
    auto initialized = backend.Initialize(record);
    REQUIRE_FALSE(initialized.has_value());
    REQUIRE(initialized.error().Message().contains("-5"));
}

TEST_CASE("Wasmtime engine enforces oversize host log payload") {
    const fs::path root = MakeTempDir("oversize_log");
    const fs::path source = root / "extension.c";
    WriteFile(source, R"c(
__attribute__((import_module("woki_host"), import_name("host_log")))
extern int host_log(unsigned level, const char* message, unsigned len);
char memory_anchor[8192];
static const char message[4097] = { 'x' };
__attribute__((export_name("ext_api_version"))) unsigned ext_api_version(void) { return 1; }
__attribute__((export_name("ext_init"))) int ext_init(void) { return host_log(1, message, 4097); }
__attribute__((export_name("ext_on_tick"))) void ext_on_tick(double delta_ms) { (void)delta_ms; }
__attribute__((export_name("ext_on_event"))) void ext_on_event(unsigned t, unsigned p, unsigned l) { (void)t; (void)p; (void)l; }
__attribute__((export_name("ext_on_unload"))) void ext_on_unload(void) {}
)c");
    REQUIRE(CompileWasm(source, root / "extension.wasm", StandardExports()));

    auto record = MakeRecord(root);
    auto backend = MakeBackend();
    REQUIRE(backend.Load(record).has_value());
    auto initialized = backend.Initialize(record);
    REQUIRE_FALSE(initialized.has_value());
    REQUIRE(initialized.error().Message().contains("-3"));
}

#endif // WOKI_EXTENSION_WITH_WASMTIME
