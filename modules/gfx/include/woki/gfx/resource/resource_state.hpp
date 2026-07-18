#pragma once

#include <string_view>

#include <woki/core.hpp>

namespace woki::gfx {

enum class ResourceState : u8 {
    Unloaded = 0,
    LoadingCpu,
    CpuReady,
    UploadQueued,
    Resident,
    ReplacementPending,
    Failed,
    Retiring,
};

[[nodiscard]] constexpr std::string_view ToString(const ResourceState state) noexcept {
    switch (state) {
    case ResourceState::Unloaded:
        return "Unloaded";
    case ResourceState::LoadingCpu:
        return "LoadingCpu";
    case ResourceState::CpuReady:
        return "CpuReady";
    case ResourceState::UploadQueued:
        return "UploadQueued";
    case ResourceState::Resident:
        return "Resident";
    case ResourceState::ReplacementPending:
        return "ReplacementPending";
    case ResourceState::Failed:
        return "Failed";
    case ResourceState::Retiring:
        return "Retiring";
    }

    return "Unknown";
}

} // namespace woki::gfx
