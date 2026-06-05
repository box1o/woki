#pragma once

#include <optional>
#include <string>
#include <vector>

#include <woki/core.hpp>

namespace woki::api {

// Common
enum class Backend : u8 {
    kAuto = 0,
    kWebGpu,
    kVulkan,
    kMetal,
    kD3D12,
    kOpenGl,
    kOpenGles,
    kNull,
};

// Instance

enum class InstanceFeature : u8 {
    kTimedWaitAny = 0,
    kShaderSourceSpirv,
    kMultipleDevicesPerAdapter,
};

struct InstanceLimits {
    u64 timed_wait_any_max_count{0};
};

struct InstanceDesc {
    Backend backend{Backend::kAuto};
    std::vector<InstanceFeature> required_features{};
    std::optional<InstanceLimits> required_limits{};
    bool enable_validation{true};
    bool enable_debug_labels{true};
    std::string label{"Instance"};
};
} // namespace woki::api
