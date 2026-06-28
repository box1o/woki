#include "wokiext/cli.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace wokiext {

namespace {

namespace fs = std::filesystem;

[[nodiscard]] std::string Slug(std::string_view text, char separator) {
    std::string out;
    bool previous_separator = false;
    for (const char raw_ch : text) {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isalnum(ch) != 0) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            previous_separator = false;
            continue;
        }
        if (!previous_separator && !out.empty()) {
            out.push_back(separator);
            previous_separator = true;
        }
    }
    while (!out.empty() && out.back() == separator) {
        out.pop_back();
    }
    return out.empty() ? "extension" : out;
}

[[nodiscard]] std::string ToId(std::string_view name) { return "woki." + Slug(name, '.'); }

void WriteFile(const fs::path& path, std::string_view contents) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.good()) {
        throw std::runtime_error("Failed to create file: " + path.string());
    }
    output << contents;
}

[[nodiscard]] std::string Manifest(std::string_view id, std::string_view name) {
    return "id: " + std::string(id) + R"yaml(
name: )yaml" +
           std::string(name) + R"yaml(
version: 0.1.0
apiVersion: 1
runtime:
  wasm: extension.wasm
permissions:
  - log
)yaml";
}

[[nodiscard]] std::string PluginCpp() {
    return R"cpp(#include "types.h"
#include "version.h"

#include <stdint.h>

#if defined(__clang__)
#define WOKI_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define WOKI_EXPORT(name) __attribute__((export_name(name)))
#else
#define WOKI_IMPORT(module, name)
#define WOKI_EXPORT(name)
#endif

extern "C" {

WOKI_IMPORT("woki_host", "host_log")
int32_t host_log(woki_ext_log_level_t level, const char* message, uint32_t len);

static uint8_t g_event_payload[4096];

WOKI_EXPORT("ext_alloc")
uint32_t ext_alloc(uint32_t len) {
    if (len == 0 || len > sizeof(g_event_payload)) {
        return 0;
    }
    return reinterpret_cast<uint32_t>(g_event_payload);
}

WOKI_EXPORT("ext_free")
void ext_free(uint32_t ptr, uint32_t len) {
    (void)ptr;
    (void)len;
}

WOKI_EXPORT("ext_api_version")
uint32_t ext_api_version(void) { return WOKI_EXT_API_VERSION; }

WOKI_EXPORT("ext_init")
int32_t ext_init(void) {
    static constexpr char kMessage[] = "hello from wokiext";
    return host_log(WOKI_EXT_LOG_INFO, kMessage, sizeof(kMessage) - 1);
}

WOKI_EXPORT("ext_on_tick")
void ext_on_tick(double dt_ms) { (void)dt_ms; }

WOKI_EXPORT("ext_on_event")
void ext_on_event(uint32_t type, const uint8_t* payload, uint32_t len) {
    (void)type;
    (void)payload;
    (void)len;
}

WOKI_EXPORT("ext_on_unload")
void ext_on_unload(void) {}

} // extern "C"
)cpp";
}

[[nodiscard]] std::string PluginC() {
    return R"c(#include "types.h"
#include "version.h"

#include <stdint.h>

#if defined(__clang__)
#define WOKI_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define WOKI_EXPORT(name) __attribute__((export_name(name)))
#else
#define WOKI_IMPORT(module, name)
#define WOKI_EXPORT(name)
#endif

WOKI_IMPORT("woki_host", "host_log")
int32_t host_log(woki_ext_log_level_t level, const char* message, uint32_t len);

static uint8_t g_event_payload[4096];

WOKI_EXPORT("ext_alloc")
uint32_t ext_alloc(uint32_t len) {
    if (len == 0 || len > sizeof(g_event_payload)) {
        return 0;
    }
    return (uint32_t)g_event_payload;
}

WOKI_EXPORT("ext_free")
void ext_free(uint32_t ptr, uint32_t len) {
    (void)ptr;
    (void)len;
}

WOKI_EXPORT("ext_api_version")
uint32_t ext_api_version(void) { return WOKI_EXT_API_VERSION; }

WOKI_EXPORT("ext_init")
int32_t ext_init(void) {
    static const char message[] = "hello from wokiext";
    return host_log(WOKI_EXT_LOG_INFO, message, sizeof(message) - 1);
}

WOKI_EXPORT("ext_on_tick")
void ext_on_tick(double dt_ms) { (void)dt_ms; }

WOKI_EXPORT("ext_on_event")
void ext_on_event(uint32_t type, const uint8_t* payload, uint32_t len) {
    (void)type;
    (void)payload;
    (void)len;
}

WOKI_EXPORT("ext_on_unload")
void ext_on_unload(void) {}
)c";
}

[[nodiscard]] std::string ExtensionCMake(std::string_view lang) {
    const bool cpp = lang == "cpp";
    const std::string compiler = cpp ? "clang++" : "clang";
    const std::string lang_token = cpp ? "CXX" : "C";
    const std::string source = cpp ? "src/plugin.cpp" : "src/plugin.c";

    return "cmake_minimum_required(VERSION 3.25)\n"
           "find_program(WOKI_WASM_COMPILER " +
           compiler + " REQUIRED)\n"
           "set(CMAKE_" +
           lang_token + R"(_COMPILER "${WOKI_WASM_COMPILER}" CACHE STRING "Wasm extension compiler" FORCE)
project(woki_extension LANGUAGES )" +
           lang_token + R"()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(WOKI_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
if(EXISTS "${WOKI_REPO_ROOT}/cmake/ExtensionWasm.cmake")
    include(${WOKI_REPO_ROOT}/cmake/ExtensionWasm.cmake)
    woki_add_extension_wasm()" +
           source + R"()
else()
    message(FATAL_ERROR
        "woki_add_extension_wasm requires cmake/ExtensionWasm.cmake. "
        "Create extensions inside the woki repository under extensions/.")
endif()
)";
}

[[nodiscard]] std::string ExtensionMakefile() {
    return R"make(BUILD_DIR ?= build
BUILD_TYPE ?= Release
NUM_JOBS ?= $(shell nproc)

.DEFAULT_GOAL := build
.PHONY: configure build lsp verify bundle install clean

configure:
	@cmake -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	@cmake --build $(BUILD_DIR) -j$(NUM_JOBS)

lsp: configure
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "✓ LSP: compile_commands.json -> $(BUILD_DIR)/compile_commands.json"

verify: build
	@wokiext verify .

bundle: verify
	@wokiext bundle .

install: bundle
	@wokiext install .

clean:
	@rm -rf $(BUILD_DIR) extension.wasm *.wokiext
)make";
}

} // namespace

Status Create(const CreateOptions& options) {
    if (options.name.empty()) {
        std::cerr << "Extension name is required\n";
        return Status::Usage;
    }
    if (options.lang != "c" && options.lang != "cpp") {
        std::cerr << "--lang must be c or cpp\n";
        return Status::Usage;
    }

    const std::string dir_name = Slug(options.name, '-');
    const std::string id = options.id.empty() ? ToId(options.name) : options.id;
    const fs::path root = options.out_dir / dir_name;

    std::error_code error;
    if (fs::exists(root, error)) {
        std::cerr << "Refusing to overwrite existing directory: " << root << '\n';
        return Status::Error;
    }

    fs::create_directories(root / "src", error);
    if (error) {
        std::cerr << error.message() << '\n';
        return Status::Error;
    }

    WriteFile(root / "manifest.yaml", Manifest(id, options.name));
    WriteFile(root / "CMakeLists.txt", ExtensionCMake(options.lang));
    WriteFile(root / "Makefile", ExtensionMakefile());
    WriteFile(root / "src" / (options.lang == "cpp" ? "plugin.cpp" : "plugin.c"),
        options.lang == "cpp" ? PluginCpp() : PluginC());

    std::cout << "Created extension project: " << root << '\n';
    return Status::Ok;
}

} // namespace wokiext
