#include "woki/ext/wasm/backend.hpp"

#include "woki/ext/package.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <string>

namespace woki::ext::wasm {

namespace {

namespace fs = std::filesystem;

[[nodiscard]] Result<void> ValidateWasmHeader(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.good()) {
        return Err(ErrorCode::FileReadError, "Failed to open wasm module: " + path.string());
    }

    std::array<unsigned char, 4> magic{};
    input.read(reinterpret_cast<char*>(magic.data()), static_cast<std::streamsize>(magic.size()));
    if (input.gcount() != static_cast<std::streamsize>(magic.size())) {
        return Err(ErrorCode::ParseInvalidFormat, "Wasm module is too small: " + path.string());
    }

    constexpr std::array<unsigned char, 4> kWasmMagic{0x00, 0x61, 0x73, 0x6d};
    if (magic != kWasmMagic) {
        return Err(ErrorCode::ParseInvalidFormat,
            "Wasm module has invalid magic header. Expected '\\0asm': " + path.string());
    }

    return Ok();
}

[[nodiscard]] Result<void> RequireEngine(const Engine* engine) {
    if (engine == nullptr) {
        return Err(ErrorCode::InvalidState,
            "Wasm backend has no engine. Provide a Wasmtime/Wasmer engine implementation.");
    }
    return Ok();
}

} // namespace

Backend::Backend(scope<Engine> engine) noexcept : engine_(std::move(engine)) {}

Result<void> Backend::Load(Record& record) {
    auto engine = RequireEngine(engine_.get());
    if (!engine) {
        return Err(engine.error());
    }

    auto layout = ValidatePackageLayout(record.package);
    if (!layout) {
        return Err(layout.error());
    }

    auto header = ValidateWasmHeader(record.package.wasm);
    if (!header) {
        return Err(header.error());
    }

    host::HostApi host(record);
    auto loaded = engine_->Load(record, host);
    if (!loaded) {
        return Err(loaded.error());
    }

    auto api_version = engine_->ApiVersion(record);
    if (!api_version) {
        return Err(api_version.error());
    }
    if (*api_version != record.manifest.api_version) {
        return Err(ErrorCode::ValidationInvalidState,
            "Extension apiVersion mismatch. Manifest declares " +
                std::to_string(record.manifest.api_version) + ", wasm exports " +
                std::to_string(*api_version) + ".");
    }

    record.tier = RuntimeTier::Wasm;
    return Ok();
}

Result<void> Backend::Initialize(Record& record) {
    auto engine = RequireEngine(engine_.get());
    if (!engine) {
        return Err(engine.error());
    }

    auto result = engine_->Init(record);
    if (!result) {
        return Err(result.error());
    }
    if (*result != 0) {
        return Err(ErrorCode::ValidationInvalidState,
            "Extension ext_init returned failure code " + std::to_string(*result) + ".");
    }

    return Ok();
}

void Backend::Tick(Record& record, f64 delta_ms) {
    if (engine_ == nullptr) {
        return;
    }
    auto ticked = engine_->Tick(record, delta_ms);
    if (!ticked) {
        record.state = State::Failed;
        record.error = std::string(ticked.error().Message());
    }
}

void Backend::DispatchEvent(Record& record, u32 event_type, std::span<const u8> payload) {
    if (engine_ == nullptr) {
        return;
    }
    auto dispatched = engine_->Event(record, event_type, payload);
    if (!dispatched) {
        record.state = State::Failed;
        record.error = std::string(dispatched.error().Message());
    }
}

void Backend::Unload(Record& record) {
    if (engine_ == nullptr) {
        return;
    }
    engine_->Unload(record);
}

} // namespace woki::ext::wasm
