#pragma once

#include <string_view>

#include <woki/core.hpp>

namespace woki::gfx {

enum class State : u8 {
    Unloaded = 0,
    LoadingCpu,
    CpuReady,
    UploadQueued,
    Resident,
    ReplacementPending,
    Failed,
    Retiring,
};

[[nodiscard]] constexpr std::string_view ToString(const State state) noexcept {
    switch (state) {
    case State::Unloaded:
        return "Unloaded";
    case State::LoadingCpu:
        return "LoadingCpu";
    case State::CpuReady:
        return "CpuReady";
    case State::UploadQueued:
        return "UploadQueued";
    case State::Resident:
        return "Resident";
    case State::ReplacementPending:
        return "ReplacementPending";
    case State::Failed:
        return "Failed";
    case State::Retiring:
        return "Retiring";
    }

    return "Unknown";
}

} // namespace woki::gfx
