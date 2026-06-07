#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <woki/core.hpp>

namespace woki {
class Window;
}

namespace woki::api {

class Adapter;
class Device;
class Instance;
class Surface;
class Swapchain;

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

enum class AdapterType : u8 { kDiscreteGpu, kIntegratedGpu, kCpu, kUnknown };

enum class BackendType : u8 {
    kUndefined,
    kNull,
    kWebGpu,
    kD3D11,
    kD3D12,
    kMetal,
    kVulkan,
    kOpenGl,
    kOpenGles,
};

enum class CallbackMode : u8 { kWaitAnyOnly, kAllowProcessEvents, kAllowSpontaneous };

enum class DeviceLostReason : u8 { kUnknown, kDestroyed, kCallbackCancelled, kFailedCreation };

enum class ErrorType : u8 { kNoError, kValidation, kOutOfMemory, kInternal, kUnknown, kDeviceLost };

enum class FeatureLevel : u8 { kUndefined, kCore, kCompatibility };

enum class FeatureName : u16 {
    kUndefined,
    kCoreFeaturesAndLimits,
    kDepthClipControl,
    kDepth32FloatStencil8,
    kTextureCompressionBc,
    kTextureCompressionBcSliced3D,
    kTextureCompressionEtc2,
    kTextureCompressionAstc,
    kTextureCompressionAstcSliced3D,
    kTimestampQuery,
    kIndirectFirstInstance,
    kShaderF16,
    kRg11B10UfloatRenderable,
    kBGRA8UnormStorage,
    kFloat32Filterable,
    kFloat32Blendable,
    kClipDistances,
    kDualSourceBlending,
    kSubgroups,
    kTextureFormatsTier1,
    kTextureFormatsTier2,
    kPrimitiveIndex,
    kTextureComponentSwizzle,
    kDawnInternalUsages,
    kDawnMultiPlanarFormats,
    kDawnNative,
    kChromiumExperimentalTimestampQueryInsidePasses,
    kImplicitDeviceSynchronization,
    kTransientAttachments,
    kMsaaRenderToSingleSampled,
    kD3D11MultithreadProtected,
    kAngleTextureSharing,
    kPixelLocalStorageCoherent,
    kPixelLocalStorageNonCoherent,
    kUnorm16TextureFormats,
    kMultiPlanarFormatExtendedUsages,
    kMultiPlanarFormatP010,
    kHostMappedPointer,
    kMultiPlanarRenderTargets,
    kMultiPlanarFormatNv12a,
    kFramebufferFetch,
    kBufferMapExtendedUsages,
    kAdapterPropertiesMemoryHeaps,
    kAdapterPropertiesD3D,
    kAdapterPropertiesVk,
    kDawnFormatCapabilities,
    kDawnDrmFormatCapabilities,
    kMultiPlanarFormatNv16,
    kMultiPlanarFormatNv24,
    kMultiPlanarFormatP210,
    kMultiPlanarFormatP410,
    kSharedTextureMemoryVkDedicatedAllocation,
    kSharedTextureMemoryAHardwareBuffer,
    kSharedTextureMemoryDmaBuf,
    kSharedTextureMemoryOpaqueFd,
    kSharedTextureMemoryZirconHandle,
    kSharedTextureMemoryDxgiSharedHandle,
    kSharedTextureMemoryD3D11Texture2D,
    kSharedTextureMemoryIosSurface,
    kSharedTextureMemoryEglImage,
    kSharedFenceVkSemaphoreOpaqueFd,
    kSharedFenceSyncFd,
    kSharedFenceVkSemaphoreZirconHandle,
    kSharedFenceDxgiSharedHandle,
    kSharedFenceMtlSharedEvent,
    kSharedBufferMemoryD3D12Resource,
    kStaticSamplers,
    kYCbCrVulkanSamplers,
    kShaderModuleCompilationOptions,
    kDawnLoadResolveTexture,
    kDawnPartialLoadResolveTexture,
    kMultiDrawIndirect,
    kDawnTexelCopyBufferRowAlignment,
    kFlexibleTextureViews,
    kChromiumExperimentalSubgroupMatrix,
    kSharedFenceEglSync,
    kDawnDeviceAllocatorControl,
    kAdapterPropertiesWgpu,
    kSharedBufferMemoryD3D12SharedMemoryFileMappingHandle,
    kSharedTextureMemoryD3D12Resource,
    kChromiumExperimentalSamplingResourceTable,
    kSubgroupSizeControl,
    kAtomicVec2uMinMax,
    kUnorm16FormatsForExternalTexture,
    kOpaqueYCbCrAndroidForExternalTexture,
    kUnorm16Filterable,
    kRenderPassRenderArea,
    kAdapterPropertiesDrm,
};

enum class TextureFormat : u16 {
    kUndefined = 0,
    kRgba8Unorm,
    kRgba8UnormSrgb,
    kBgra8Unorm,
    kBgra8UnormSrgb,
    kDepth16Unorm,
    kDepth24Plus,
    kDepth24PlusStencil8,
    kDepth32Float,
    kDepth32FloatStencil8,
};

enum class PresentMode : u8 { kUndefined, kFifo, kFifoRelaxed, kImmediate, kMailbox };

enum class CompositeAlphaMode : u8 { kAuto, kOpaque, kPremultiplied, kUnpremultiplied, kInherit };

enum class SurfaceGetCurrentTextureStatus : u8 {
    kSuccessOptimal,
    kSuccessSuboptimal,
    kTimeout,
    kOutdated,
    kLost,
    kError,
};

enum class TextureUsage : u32 {
    kNone = 0,
    kCopySrc = 1 << 0,
    kCopyDst = 1 << 1,
    kTextureBinding = 1 << 2,
    kStorageBinding = 1 << 3,
    kRenderAttachment = 1 << 4,
};

// Instance

enum class InstanceFeature : u8 {
    kTimedWaitAny = 0,
    kShaderSourceSpirv,
    kMultipleDevicesPerAdapter,
};

enum class PowerPreference : u8 { kUndefined, kLowPower, kHighPerformance };

enum class RequestAdapterStatus : u8 { kSuccess, kCallbackCancelled, kUnavailable, kError };

enum class RequestDeviceStatus : u8 { kSuccess, kCallbackCancelled, kError };

enum class Status : u8 { kSuccess, kError };

enum class WaitStatus : u8 {
    kSuccess,
    kTimedOut,
    kUnsupportedTimeout,
    kUnsupportedCount,
    kUnsupportedMixedSources,
    kUnknown,
    kError,
};

struct Future final {
    bool completed{false};
    bool success{false};
    std::string message{};
    u64 id{0};
};

struct FutureWaitInfo {
    Future future{};
    bool completed{false};
};

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept {
    return static_cast<TextureUsage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

[[nodiscard]] constexpr bool HasFlag(TextureUsage value, TextureUsage flag) noexcept {
    return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
}

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

struct AdapterInfo {
    std::string vendor{};
    std::string architecture{};
    std::string device{};
    std::string description{};
    BackendType backend_type{BackendType::kUndefined};
    AdapterType adapter_type{AdapterType::kUnknown};
    FeatureLevel feature_level{FeatureLevel::kCore};
    u32 vendor_id{0};
    u32 device_id{0};
    u32 subgroup_min_size{0};
    u32 subgroup_max_size{0};
};

struct Limits {
    u32 max_texture_dimension_1d{0};
    u32 max_texture_dimension_2d{0};
    u32 max_texture_dimension_3d{0};
    u32 max_texture_array_layers{0};
    u32 max_bind_groups{0};
    u32 max_bind_groups_plus_vertex_buffers{0};
    u32 max_bindings_per_bind_group{0};
    u32 max_dynamic_uniform_buffers_per_pipeline_layout{0};
    u32 max_dynamic_storage_buffers_per_pipeline_layout{0};
    u32 max_sampled_textures_per_shader_stage{0};
    u32 max_samplers_per_shader_stage{0};
    u32 max_storage_buffers_per_shader_stage{0};
    u32 max_storage_textures_per_shader_stage{0};
    u32 max_uniform_buffers_per_shader_stage{0};
    u64 max_uniform_buffer_binding_size{0};
    u64 max_storage_buffer_binding_size{0};
    u32 min_uniform_buffer_offset_alignment{0};
    u32 min_storage_buffer_offset_alignment{0};
    u32 max_vertex_buffers{0};
    u64 max_buffer_size{0};
    u32 max_vertex_attributes{0};
    u32 max_vertex_buffer_array_stride{0};
    u32 max_inter_stage_shader_components{0};
    u32 max_inter_stage_shader_variables{0};
    u32 max_color_attachments{0};
    u32 max_color_attachment_bytes_per_sample{0};
    u32 max_compute_workgroup_storage_size{0};
    u32 max_compute_invocations_per_workgroup{0};
    u32 max_compute_workgroup_size_x{0};
    u32 max_compute_workgroup_size_y{0};
    u32 max_compute_workgroup_size_z{0};
    u32 max_compute_workgroups_per_dimension{0};
    u32 max_immediate_size{0};
};

struct SupportedFeatures {
    std::vector<FeatureName> values{};

    [[nodiscard]] bool Has(FeatureName value) const noexcept {
        return std::ranges::find(values, value) != values.end();
    }
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

struct RequestAdapterDesc {
    Surface* compatible_surface{nullptr};
    FeatureLevel feature_level{FeatureLevel::kUndefined};
    BackendType backend_type{BackendType::kUndefined};
    PowerPreference power_preference{PowerPreference::kHighPerformance};
    bool force_fallback_adapter{false};
};

struct QueueDesc {
    std::string label{"Queue"};
};

struct DeviceDesc {
    Backend backend{Backend::kAuto};
    bool enable_validation{true};
    bool enable_debug_labels{true};
    std::vector<FeatureName> required_features{};
    std::optional<Limits> required_limits{};
    QueueDesc default_queue{};
    DeviceLostCallback device_lost_callback{};
    UncapturedErrorCallback uncaptured_error_callback{};
    std::string label{"Device"};
};

struct SurfaceDesc {
    Window* window{nullptr};
    std::string label{"Surface"};
};

struct SurfaceCapabilities {
    TextureUsage usages{TextureUsage::kNone};
    std::vector<TextureFormat> formats{};
    std::vector<PresentMode> present_modes{};
    std::vector<CompositeAlphaMode> alpha_modes{};
};

struct SurfaceConfiguration {
    Device* device{nullptr};
    TextureFormat format{TextureFormat::kBgra8Unorm};
    TextureUsage usage{TextureUsage::kRenderAttachment};
    u32 width{0};
    u32 height{0};
    std::vector<TextureFormat> view_formats{};
    CompositeAlphaMode alpha_mode{CompositeAlphaMode::kAuto};
    PresentMode present_mode{PresentMode::kFifo};
};

struct SurfaceTexture {
    SurfaceGetCurrentTextureStatus status{SurfaceGetCurrentTextureStatus::kError};
    NativeHandles handles{};
    bool suboptimal{false};
};

struct SwapchainDesc {
    Surface* surface{nullptr};
    u32 width{0};
    u32 height{0};
    TextureFormat format{TextureFormat::kBgra8Unorm};
    TextureFormat depth_format{TextureFormat::kDepth24PlusStencil8};
    TextureUsage usage{TextureUsage::kRenderAttachment};
    PresentMode present_mode{PresentMode::kFifo};
    CompositeAlphaMode alpha_mode{CompositeAlphaMode::kAuto};
    bool enable_depth{true};
    std::string label{"Swapchain"};
};

struct SwapchainFrame {
    NativeHandles color{};
    NativeHandles depth{};
    u32 width{0};
    u32 height{0};
};
} // namespace woki::api
