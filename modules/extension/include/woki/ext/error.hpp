#pragma once

// IWYU pragma: private, include "woki/ext/ext.hpp"

#include <woki/core.hpp>

namespace woki::ext {

enum class State : u8 {
    Discovered,
    ManifestValidated,
    PermissionChecked,
    Loaded,
    Initialized,
    Active,
    Unloading,
    Unloaded,
    Failed,
};

enum class RuntimeTier : u8 {
    None,
    Wasm,
    Native,
};

} // namespace woki::ext
