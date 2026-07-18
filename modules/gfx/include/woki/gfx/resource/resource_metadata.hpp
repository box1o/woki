#pragma once

#include "resource_id.hpp"
#include "resource_state.hpp"

#include <string>

namespace woki::gfx {

struct ResourceMetadata final {
    AssetId asset_id{};
    ResourceVersion version{};
    ResourceState state{ResourceState::Unloaded};
    std::string label{};

    [[nodiscard]] bool IsResident() const noexcept {
        return state == ResourceState::Resident || state == ResourceState::ReplacementPending;
    }
};

} // namespace woki::gfx
