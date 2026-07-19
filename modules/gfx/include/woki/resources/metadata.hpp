#pragma once

#include "asset.hpp"
#include "state.hpp"

#include <string>

namespace woki::gfx {

struct ResourceMetadata final {
    AssetId     asset_id{};
    Version     version{};
    State       state{State::Unloaded};
    std::string label{};

    [[nodiscard]] bool IsResident() const noexcept {
        return state == State::Resident || state == State::ReplacementPending;
    }
};

} // namespace woki::gfx
