#pragma once

#include "forward.hpp"

#include "../enums.hpp"

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <woki/core.hpp>

namespace woki::rhi {

struct Future final {
    u64 id{0};
    bool completed{false};
    bool success{false};
    std::string message{};
};

struct FutureWaitInfo final {
    Future future{};
    bool completed{false};
};

struct InstanceLimits final {
    u64 timed_wait_any_max_count{0};
};

struct SupportedInstanceFeatures final {
    std::vector<InstanceFeatureName> features{};

    [[nodiscard]] bool Has(InstanceFeatureName feature) const noexcept {
        return std::ranges::find(features, feature) != features.end();
    }
};

struct SupportedWGSLLanguageFeatures final {
    std::vector<WGSLLanguageFeatureName> features{};

    [[nodiscard]] bool Has(WGSLLanguageFeatureName feature) const noexcept {
        return std::ranges::find(features, feature) != features.end();
    }
};

struct NativeHandles final {
    void* instance{nullptr};
    void* adapter{nullptr};
    void* device{nullptr};
    void* queue{nullptr};
    void* surface{nullptr};
    void* resource{nullptr};
};

using RequestAdapterCallback =
    std::function<void(RequestAdapterStatus status, scope<Adapter> adapter, std::string_view message)>;

using RequestDeviceCallback =
    std::function<void(RequestDeviceStatus status, scope<Device> device, std::string_view message)>;

using DeviceLostCallback = std::function<void(DeviceLostReason reason, std::string_view message)>;
using UncapturedErrorCallback = std::function<void(ErrorType type, std::string_view message)>;

using PopErrorScopeCallback =
    std::function<void(PopErrorScopeStatus status, ErrorType type, std::string_view message)>;

using QueueWorkDoneCallback =
    std::function<void(QueueWorkDoneStatus status, std::string_view message)>;

using CreateComputePipelineCallback = std::function<void(
    CreatePipelineAsyncStatus status, scope<ComputePipeline> pipeline, std::string_view message)>;

using CreateRenderPipelineCallback = std::function<void(
    CreatePipelineAsyncStatus status, scope<RenderPipeline> pipeline, std::string_view message)>;

using LoggingCallback = std::function<void(LoggingType type, std::string_view message)>;

using Proc = void(*)();

inline constexpr u64 kWholeSize = ~0ULL;

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<u64>(lhs) | static_cast<u64>(rhs));
}

[[nodiscard]] constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<u64>(lhs) & static_cast<u64>(rhs));
}

[[nodiscard]] constexpr bool HasFlag(TextureUsage value, TextureUsage flag) noexcept {
    return (static_cast<u64>(value) & static_cast<u64>(flag)) != 0;
}

} // namespace woki::rhi
