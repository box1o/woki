#pragma once

#include "../../include/woki/api/descriptors.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <webgpu/webgpu.h>

namespace woki::api::wgpu::detail {

struct DeviceDescriptorNativeStorage {
    std::vector<WGPUFeatureName> required_features{};
    WGPULimits required_limits = WGPU_LIMITS_INIT;
    WGPUQueueDescriptor default_queue = WGPU_QUEUE_DESCRIPTOR_INIT;
    WGPUDeviceDescriptor native_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    std::shared_ptr<DeviceLostCallback> device_lost_callback{};
    std::shared_ptr<UncapturedErrorCallback> uncaptured_error_callback{};
};

[[nodiscard]] constexpr WGPUTextureFormat ToWgpuTextureFormat(TextureFormat value) noexcept {
    switch (value) {
    case TextureFormat::kRgba8Unorm:
        return WGPUTextureFormat_RGBA8Unorm;
    case TextureFormat::kRgba8UnormSrgb:
        return WGPUTextureFormat_RGBA8UnormSrgb;
    case TextureFormat::kBgra8Unorm:
        return WGPUTextureFormat_BGRA8Unorm;
    case TextureFormat::kBgra8UnormSrgb:
        return WGPUTextureFormat_BGRA8UnormSrgb;
    case TextureFormat::kDepth16Unorm:
        return WGPUTextureFormat_Depth16Unorm;
    case TextureFormat::kDepth24Plus:
        return WGPUTextureFormat_Depth24Plus;
    case TextureFormat::kDepth24PlusStencil8:
        return WGPUTextureFormat_Depth24PlusStencil8;
    case TextureFormat::kDepth32Float:
        return WGPUTextureFormat_Depth32Float;
    case TextureFormat::kDepth32FloatStencil8:
        return WGPUTextureFormat_Depth32FloatStencil8;
    case TextureFormat::kUndefined:
        return WGPUTextureFormat_Undefined;
    }

    return WGPUTextureFormat_Undefined;
}

[[nodiscard]] constexpr TextureFormat FromWgpuTextureFormat(WGPUTextureFormat value) noexcept {
    switch (value) {
    case WGPUTextureFormat_RGBA8Unorm:
        return TextureFormat::kRgba8Unorm;
    case WGPUTextureFormat_RGBA8UnormSrgb:
        return TextureFormat::kRgba8UnormSrgb;
    case WGPUTextureFormat_BGRA8Unorm:
        return TextureFormat::kBgra8Unorm;
    case WGPUTextureFormat_BGRA8UnormSrgb:
        return TextureFormat::kBgra8UnormSrgb;
    case WGPUTextureFormat_Depth16Unorm:
        return TextureFormat::kDepth16Unorm;
    case WGPUTextureFormat_Depth24Plus:
        return TextureFormat::kDepth24Plus;
    case WGPUTextureFormat_Depth24PlusStencil8:
        return TextureFormat::kDepth24PlusStencil8;
    case WGPUTextureFormat_Depth32Float:
        return TextureFormat::kDepth32Float;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return TextureFormat::kDepth32FloatStencil8;
    case WGPUTextureFormat_Undefined:
    case WGPUTextureFormat_Force32:
        return TextureFormat::kUndefined;
    default:
        return TextureFormat::kUndefined;
    }
}

[[nodiscard]] constexpr WGPUPresentMode ToWgpuPresentMode(PresentMode value) noexcept {
    switch (value) {
    case PresentMode::kFifo:
        return WGPUPresentMode_Fifo;
    case PresentMode::kFifoRelaxed:
        return WGPUPresentMode_FifoRelaxed;
    case PresentMode::kImmediate:
        return WGPUPresentMode_Immediate;
    case PresentMode::kMailbox:
        return WGPUPresentMode_Mailbox;
    case PresentMode::kUndefined:
        return WGPUPresentMode_Undefined;
    }

    return WGPUPresentMode_Undefined;
}

[[nodiscard]] constexpr PresentMode FromWgpuPresentMode(WGPUPresentMode value) noexcept {
    switch (value) {
    case WGPUPresentMode_Fifo:
        return PresentMode::kFifo;
    case WGPUPresentMode_FifoRelaxed:
        return PresentMode::kFifoRelaxed;
    case WGPUPresentMode_Immediate:
        return PresentMode::kImmediate;
    case WGPUPresentMode_Mailbox:
        return PresentMode::kMailbox;
    case WGPUPresentMode_Undefined:
    case WGPUPresentMode_Force32:
        return PresentMode::kUndefined;
    }

    return PresentMode::kUndefined;
}

[[nodiscard]] constexpr WGPUCompositeAlphaMode ToWgpuCompositeAlphaMode(
    CompositeAlphaMode value) noexcept {
    switch (value) {
    case CompositeAlphaMode::kAuto:
        return WGPUCompositeAlphaMode_Auto;
    case CompositeAlphaMode::kOpaque:
        return WGPUCompositeAlphaMode_Opaque;
    case CompositeAlphaMode::kPremultiplied:
        return WGPUCompositeAlphaMode_Premultiplied;
    case CompositeAlphaMode::kUnpremultiplied:
        return WGPUCompositeAlphaMode_Unpremultiplied;
    case CompositeAlphaMode::kInherit:
        return WGPUCompositeAlphaMode_Inherit;
    }

    return WGPUCompositeAlphaMode_Auto;
}

[[nodiscard]] constexpr CompositeAlphaMode FromWgpuCompositeAlphaMode(
    WGPUCompositeAlphaMode value) noexcept {
    switch (value) {
    case WGPUCompositeAlphaMode_Auto:
        return CompositeAlphaMode::kAuto;
    case WGPUCompositeAlphaMode_Opaque:
        return CompositeAlphaMode::kOpaque;
    case WGPUCompositeAlphaMode_Premultiplied:
        return CompositeAlphaMode::kPremultiplied;
    case WGPUCompositeAlphaMode_Unpremultiplied:
        return CompositeAlphaMode::kUnpremultiplied;
    case WGPUCompositeAlphaMode_Inherit:
        return CompositeAlphaMode::kInherit;
    case WGPUCompositeAlphaMode_Force32:
        break;
    }

    return CompositeAlphaMode::kAuto;
}

[[nodiscard]] constexpr WGPUTextureUsage ToWgpuTextureUsage(TextureUsage value) noexcept {
    WGPUTextureUsage result = WGPUTextureUsage_None;
    if (HasFlag(value, TextureUsage::kCopySrc)) {
        result |= WGPUTextureUsage_CopySrc;
    }
    if (HasFlag(value, TextureUsage::kCopyDst)) {
        result |= WGPUTextureUsage_CopyDst;
    }
    if (HasFlag(value, TextureUsage::kTextureBinding)) {
        result |= WGPUTextureUsage_TextureBinding;
    }
    if (HasFlag(value, TextureUsage::kStorageBinding)) {
        result |= WGPUTextureUsage_StorageBinding;
    }
    if (HasFlag(value, TextureUsage::kRenderAttachment)) {
        result |= WGPUTextureUsage_RenderAttachment;
    }
    return result;
}

[[nodiscard]] constexpr TextureUsage FromWgpuTextureUsage(WGPUTextureUsage value) noexcept {
    TextureUsage result = TextureUsage::kNone;
    if ((value & WGPUTextureUsage_CopySrc) != 0) {
        result = result | TextureUsage::kCopySrc;
    }
    if ((value & WGPUTextureUsage_CopyDst) != 0) {
        result = result | TextureUsage::kCopyDst;
    }
    if ((value & WGPUTextureUsage_TextureBinding) != 0) {
        result = result | TextureUsage::kTextureBinding;
    }
    if ((value & WGPUTextureUsage_StorageBinding) != 0) {
        result = result | TextureUsage::kStorageBinding;
    }
    if ((value & WGPUTextureUsage_RenderAttachment) != 0) {
        result = result | TextureUsage::kRenderAttachment;
    }
    return result;
}

[[nodiscard]] constexpr SurfaceGetCurrentTextureStatus FromWgpuSurfaceTextureStatus(
    WGPUSurfaceGetCurrentTextureStatus value) noexcept {
    switch (value) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        return SurfaceGetCurrentTextureStatus::kSuccessOptimal;
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        return SurfaceGetCurrentTextureStatus::kSuccessSuboptimal;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        return SurfaceGetCurrentTextureStatus::kTimeout;
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        return SurfaceGetCurrentTextureStatus::kOutdated;
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        return SurfaceGetCurrentTextureStatus::kLost;
    case WGPUSurfaceGetCurrentTextureStatus_Error:
    case WGPUSurfaceGetCurrentTextureStatus_Force32:
        return SurfaceGetCurrentTextureStatus::kError;
    }

    return SurfaceGetCurrentTextureStatus::kError;
}

[[nodiscard]] inline WGPUStringView ToWgpuStringView(std::string_view value) noexcept {
    WGPUStringView result = WGPU_STRING_VIEW_INIT;
    result.data = value.data();
    result.length = value.size();
    return result;
}

[[nodiscard]] inline std::string StringFromView(WGPUStringView view) {
    if (view.data == nullptr) {
        return {};
    }

    if (view.length == WGPU_STRLEN) {
        return std::string(view.data);
    }

    return std::string(view.data, view.length);
}

[[nodiscard]] constexpr WGPUInstanceFeatureName ToWgpuInstanceFeature(InstanceFeature feature) noexcept {
    switch (feature) {
    case InstanceFeature::kTimedWaitAny:
        return WGPUInstanceFeatureName_TimedWaitAny;
    case InstanceFeature::kShaderSourceSpirv:
        return WGPUInstanceFeatureName_ShaderSourceSPIRV;
    case InstanceFeature::kMultipleDevicesPerAdapter:
        return WGPUInstanceFeatureName_MultipleDevicesPerAdapter;
    }

    return WGPUInstanceFeatureName_Force32;
}

[[nodiscard]] inline std::optional<InstanceFeature> FromWgpuInstanceFeature(
    WGPUInstanceFeatureName feature) noexcept {
    switch (feature) {
    case WGPUInstanceFeatureName_TimedWaitAny:
        return InstanceFeature::kTimedWaitAny;
    case WGPUInstanceFeatureName_ShaderSourceSPIRV:
        return InstanceFeature::kShaderSourceSpirv;
    case WGPUInstanceFeatureName_MultipleDevicesPerAdapter:
        return InstanceFeature::kMultipleDevicesPerAdapter;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] constexpr WGPUPowerPreference ToWgpuPowerPreference(PowerPreference value) noexcept {
    switch (value) {
    case PowerPreference::kLowPower:
        return WGPUPowerPreference_LowPower;
    case PowerPreference::kHighPerformance:
        return WGPUPowerPreference_HighPerformance;
    case PowerPreference::kUndefined:
        return WGPUPowerPreference_Undefined;
    }

    return WGPUPowerPreference_Undefined;
}

[[nodiscard]] constexpr WGPUFeatureLevel ToWgpuFeatureLevel(FeatureLevel value) noexcept {
    switch (value) {
    case FeatureLevel::kCore:
        return WGPUFeatureLevel_Core;
    case FeatureLevel::kCompatibility:
        return WGPUFeatureLevel_Compatibility;
    case FeatureLevel::kUndefined:
        return WGPUFeatureLevel_Undefined;
    }

    return WGPUFeatureLevel_Undefined;
}

[[nodiscard]] constexpr WGPUBackendType ToWgpuBackendType(BackendType value) noexcept {
    switch (value) {
    case BackendType::kNull:
        return WGPUBackendType_Null;
    case BackendType::kWebGpu:
        return WGPUBackendType_WebGPU;
    case BackendType::kD3D11:
        return WGPUBackendType_D3D11;
    case BackendType::kD3D12:
        return WGPUBackendType_D3D12;
    case BackendType::kMetal:
        return WGPUBackendType_Metal;
    case BackendType::kVulkan:
        return WGPUBackendType_Vulkan;
    case BackendType::kOpenGl:
        return WGPUBackendType_OpenGL;
    case BackendType::kOpenGles:
        return WGPUBackendType_OpenGLES;
    case BackendType::kUndefined:
        return WGPUBackendType_Undefined;
    }

    return WGPUBackendType_Undefined;
}

[[nodiscard]] constexpr BackendType FromWgpuBackendType(WGPUBackendType value) noexcept {
    switch (value) {
    case WGPUBackendType_Null:
        return BackendType::kNull;
    case WGPUBackendType_WebGPU:
        return BackendType::kWebGpu;
    case WGPUBackendType_D3D11:
        return BackendType::kD3D11;
    case WGPUBackendType_D3D12:
        return BackendType::kD3D12;
    case WGPUBackendType_Metal:
        return BackendType::kMetal;
    case WGPUBackendType_Vulkan:
        return BackendType::kVulkan;
    case WGPUBackendType_OpenGL:
        return BackendType::kOpenGl;
    case WGPUBackendType_OpenGLES:
        return BackendType::kOpenGles;
    case WGPUBackendType_Undefined:
    case WGPUBackendType_Force32:
        return BackendType::kUndefined;
    }

    return BackendType::kUndefined;
}

[[nodiscard]] constexpr AdapterType FromWgpuAdapterType(WGPUAdapterType value) noexcept {
    switch (value) {
    case WGPUAdapterType_DiscreteGPU:
        return AdapterType::kDiscreteGpu;
    case WGPUAdapterType_IntegratedGPU:
        return AdapterType::kIntegratedGpu;
    case WGPUAdapterType_CPU:
        return AdapterType::kCpu;
    case WGPUAdapterType_Unknown:
    case WGPUAdapterType_Force32:
        return AdapterType::kUnknown;
    }

    return AdapterType::kUnknown;
}

[[nodiscard]] constexpr CallbackMode FromWgpuCallbackMode(WGPUCallbackMode value) noexcept {
    switch (value) {
    case WGPUCallbackMode_WaitAnyOnly:
        return CallbackMode::kWaitAnyOnly;
    case WGPUCallbackMode_AllowProcessEvents:
        return CallbackMode::kAllowProcessEvents;
    case WGPUCallbackMode_AllowSpontaneous:
        return CallbackMode::kAllowSpontaneous;
    case WGPUCallbackMode_Force32:
        break;
    }

    return CallbackMode::kWaitAnyOnly;
}

[[nodiscard]] constexpr WGPUCallbackMode ToWgpuCallbackMode(CallbackMode value) noexcept {
    switch (value) {
    case CallbackMode::kWaitAnyOnly:
        return WGPUCallbackMode_WaitAnyOnly;
    case CallbackMode::kAllowProcessEvents:
        return WGPUCallbackMode_AllowProcessEvents;
    case CallbackMode::kAllowSpontaneous:
        return WGPUCallbackMode_AllowSpontaneous;
    }

    return WGPUCallbackMode_WaitAnyOnly;
}

[[nodiscard]] constexpr RequestAdapterStatus FromWgpuRequestAdapterStatus(
    WGPURequestAdapterStatus value) noexcept {
    switch (value) {
    case WGPURequestAdapterStatus_Success:
        return RequestAdapterStatus::kSuccess;
    case WGPURequestAdapterStatus_CallbackCancelled:
        return RequestAdapterStatus::kCallbackCancelled;
    case WGPURequestAdapterStatus_Unavailable:
        return RequestAdapterStatus::kUnavailable;
    case WGPURequestAdapterStatus_Error:
    case WGPURequestAdapterStatus_Force32:
        return RequestAdapterStatus::kError;
    }

    return RequestAdapterStatus::kError;
}

[[nodiscard]] constexpr RequestDeviceStatus FromWgpuRequestDeviceStatus(
    WGPURequestDeviceStatus value) noexcept {
    switch (value) {
    case WGPURequestDeviceStatus_Success:
        return RequestDeviceStatus::kSuccess;
    case WGPURequestDeviceStatus_CallbackCancelled:
        return RequestDeviceStatus::kCallbackCancelled;
    case WGPURequestDeviceStatus_Error:
    case WGPURequestDeviceStatus_Force32:
        return RequestDeviceStatus::kError;
    }

    return RequestDeviceStatus::kError;
}

[[nodiscard]] constexpr WaitStatus FromWgpuWaitStatus(WGPUWaitStatus value) noexcept {
    switch (value) {
    case WGPUWaitStatus_Success:
        return WaitStatus::kSuccess;
    case WGPUWaitStatus_TimedOut:
        return WaitStatus::kTimedOut;
    case WGPUWaitStatus_Error:
    case WGPUWaitStatus_Force32:
        return WaitStatus::kError;
    }

    return WaitStatus::kError;
}

[[nodiscard]] constexpr ErrorType FromWgpuErrorType(WGPUErrorType value) noexcept {
    switch (value) {
    case WGPUErrorType_NoError:
        return ErrorType::kNoError;
    case WGPUErrorType_Validation:
        return ErrorType::kValidation;
    case WGPUErrorType_OutOfMemory:
        return ErrorType::kOutOfMemory;
    case WGPUErrorType_Internal:
        return ErrorType::kInternal;
    case WGPUErrorType_Unknown:
        return ErrorType::kUnknown;
    case WGPUErrorType_Force32:
        break;
    }

    return ErrorType::kUnknown;
}

[[nodiscard]] constexpr DeviceLostReason FromWgpuDeviceLostReason(WGPUDeviceLostReason value) noexcept {
    switch (value) {
    case WGPUDeviceLostReason_Destroyed:
        return DeviceLostReason::kDestroyed;
    case WGPUDeviceLostReason_CallbackCancelled:
        return DeviceLostReason::kCallbackCancelled;
    case WGPUDeviceLostReason_FailedCreation:
        return DeviceLostReason::kFailedCreation;
    case WGPUDeviceLostReason_Unknown:
    case WGPUDeviceLostReason_Force32:
        return DeviceLostReason::kUnknown;
    }

    return DeviceLostReason::kUnknown;
}

[[nodiscard]] constexpr WGPUFeatureName ToWgpuFeatureName(FeatureName value) noexcept {
    if (value == FeatureName::kUndefined) {
        return WGPUFeatureName_Force32;
    }

    return static_cast<WGPUFeatureName>(value);
}

[[nodiscard]] constexpr FeatureName FromWgpuFeatureName(WGPUFeatureName value) noexcept {
    if (value == WGPUFeatureName_Force32) {
        return FeatureName::kUndefined;
    }

    return static_cast<FeatureName>(value);
}

[[nodiscard]] inline std::vector<InstanceFeature> QuerySupportedInstanceFeatures() {
    WGPUSupportedInstanceFeatures native_features = WGPU_SUPPORTED_INSTANCE_FEATURES_INIT;
    wgpuGetInstanceFeatures(&native_features);

    std::vector<InstanceFeature> features;
    features.reserve(native_features.featureCount);
    for (size_t i = 0; i < native_features.featureCount; ++i) {
        const auto feature = FromWgpuInstanceFeature(native_features.features[i]);
        if (feature.has_value()) {
            features.push_back(*feature);
        }
    }

    wgpuSupportedInstanceFeaturesFreeMembers(native_features);
    return features;
}

[[nodiscard]] inline std::vector<WGPUInstanceFeatureName> BuildRequiredInstanceFeatures(
    const std::vector<InstanceFeature>& features) {
    std::vector<WGPUInstanceFeatureName> native_features;
    native_features.reserve(features.size());
    for (const auto feature : features) {
        const auto native_feature = ToWgpuInstanceFeature(feature);
        if (!wgpuHasInstanceFeature(native_feature)) {
            return {};
        }

        native_features.push_back(native_feature);
    }

    return native_features;
}

inline void FillAdapterInfoFromNative(const WGPUAdapterInfo& native_info, AdapterInfo* info) {
    if (info == nullptr) {
        return;
    }

    info->vendor = StringFromView(native_info.vendor);
    info->architecture = StringFromView(native_info.architecture);
    info->device = StringFromView(native_info.device);
    info->description = StringFromView(native_info.description);
    info->backend_type = FromWgpuBackendType(native_info.backendType);
    info->adapter_type = FromWgpuAdapterType(native_info.adapterType);
    info->vendor_id = native_info.vendorID;
    info->device_id = native_info.deviceID;
    info->subgroup_min_size = native_info.subgroupMinSize;
    info->subgroup_max_size = native_info.subgroupMaxSize;
}

[[nodiscard]] inline Status FillAdapterInfo(WGPUAdapter adapter, AdapterInfo* info) {
    if (adapter == nullptr || info == nullptr) {
        return Status::kError;
    }

    WGPUAdapterInfo native_info = WGPU_ADAPTER_INFO_INIT;
    if (!wgpuAdapterGetInfo(adapter, &native_info)) {
        return Status::kError;
    }

    FillAdapterInfoFromNative(native_info, info);
    wgpuAdapterInfoFreeMembers(native_info);
    return Status::kSuccess;
}

[[nodiscard]] inline AdapterInfo QueryAdapterInfo(WGPUAdapter adapter) {
    AdapterInfo info{};
    static_cast<void>(FillAdapterInfo(adapter, &info));
    return info;
}

inline void FillLimitsFromNative(const WGPULimits& native_limits, Limits* limits) {
    if (limits == nullptr) {
        return;
    }

    limits->max_texture_dimension_1d = native_limits.maxTextureDimension1D;
    limits->max_texture_dimension_2d = native_limits.maxTextureDimension2D;
    limits->max_texture_dimension_3d = native_limits.maxTextureDimension3D;
    limits->max_texture_array_layers = native_limits.maxTextureArrayLayers;
    limits->max_bind_groups = native_limits.maxBindGroups;
    limits->max_bind_groups_plus_vertex_buffers = native_limits.maxBindGroupsPlusVertexBuffers;
    limits->max_bindings_per_bind_group = native_limits.maxBindingsPerBindGroup;
    limits->max_dynamic_uniform_buffers_per_pipeline_layout =
        native_limits.maxDynamicUniformBuffersPerPipelineLayout;
    limits->max_dynamic_storage_buffers_per_pipeline_layout =
        native_limits.maxDynamicStorageBuffersPerPipelineLayout;
    limits->max_sampled_textures_per_shader_stage = native_limits.maxSampledTexturesPerShaderStage;
    limits->max_samplers_per_shader_stage = native_limits.maxSamplersPerShaderStage;
    limits->max_storage_buffers_per_shader_stage = native_limits.maxStorageBuffersPerShaderStage;
    limits->max_storage_textures_per_shader_stage = native_limits.maxStorageTexturesPerShaderStage;
    limits->max_uniform_buffers_per_shader_stage = native_limits.maxUniformBuffersPerShaderStage;
    limits->max_uniform_buffer_binding_size = native_limits.maxUniformBufferBindingSize;
    limits->max_storage_buffer_binding_size = native_limits.maxStorageBufferBindingSize;
    limits->min_uniform_buffer_offset_alignment = native_limits.minUniformBufferOffsetAlignment;
    limits->min_storage_buffer_offset_alignment = native_limits.minStorageBufferOffsetAlignment;
    limits->max_vertex_buffers = native_limits.maxVertexBuffers;
    limits->max_buffer_size = native_limits.maxBufferSize;
    limits->max_vertex_attributes = native_limits.maxVertexAttributes;
    limits->max_vertex_buffer_array_stride = native_limits.maxVertexBufferArrayStride;
    limits->max_inter_stage_shader_components = native_limits.maxInterStageShaderVariables;
    limits->max_inter_stage_shader_variables = native_limits.maxInterStageShaderVariables;
    limits->max_color_attachments = native_limits.maxColorAttachments;
    limits->max_color_attachment_bytes_per_sample = native_limits.maxColorAttachmentBytesPerSample;
    limits->max_compute_workgroup_storage_size = native_limits.maxComputeWorkgroupStorageSize;
    limits->max_compute_invocations_per_workgroup = native_limits.maxComputeInvocationsPerWorkgroup;
    limits->max_compute_workgroup_size_x = native_limits.maxComputeWorkgroupSizeX;
    limits->max_compute_workgroup_size_y = native_limits.maxComputeWorkgroupSizeY;
    limits->max_compute_workgroup_size_z = native_limits.maxComputeWorkgroupSizeZ;
    limits->max_compute_workgroups_per_dimension = native_limits.maxComputeWorkgroupsPerDimension;
    limits->max_immediate_size = native_limits.maxImmediateSize;
}

[[nodiscard]] inline WGPULimits ToWgpuLimits(const Limits& limits) {
    WGPULimits native_limits = WGPU_LIMITS_INIT;
    native_limits.maxTextureDimension1D = limits.max_texture_dimension_1d;
    native_limits.maxTextureDimension2D = limits.max_texture_dimension_2d;
    native_limits.maxTextureDimension3D = limits.max_texture_dimension_3d;
    native_limits.maxTextureArrayLayers = limits.max_texture_array_layers;
    native_limits.maxBindGroups = limits.max_bind_groups;
    native_limits.maxBindGroupsPlusVertexBuffers = limits.max_bind_groups_plus_vertex_buffers;
    native_limits.maxBindingsPerBindGroup = limits.max_bindings_per_bind_group;
    native_limits.maxDynamicUniformBuffersPerPipelineLayout =
        limits.max_dynamic_uniform_buffers_per_pipeline_layout;
    native_limits.maxDynamicStorageBuffersPerPipelineLayout =
        limits.max_dynamic_storage_buffers_per_pipeline_layout;
    native_limits.maxSampledTexturesPerShaderStage = limits.max_sampled_textures_per_shader_stage;
    native_limits.maxSamplersPerShaderStage = limits.max_samplers_per_shader_stage;
    native_limits.maxStorageBuffersPerShaderStage = limits.max_storage_buffers_per_shader_stage;
    native_limits.maxStorageTexturesPerShaderStage = limits.max_storage_textures_per_shader_stage;
    native_limits.maxUniformBuffersPerShaderStage = limits.max_uniform_buffers_per_shader_stage;
    native_limits.maxUniformBufferBindingSize = limits.max_uniform_buffer_binding_size;
    native_limits.maxStorageBufferBindingSize = limits.max_storage_buffer_binding_size;
    native_limits.minUniformBufferOffsetAlignment = limits.min_uniform_buffer_offset_alignment;
    native_limits.minStorageBufferOffsetAlignment = limits.min_storage_buffer_offset_alignment;
    native_limits.maxVertexBuffers = limits.max_vertex_buffers;
    native_limits.maxBufferSize = limits.max_buffer_size;
    native_limits.maxVertexAttributes = limits.max_vertex_attributes;
    native_limits.maxVertexBufferArrayStride = limits.max_vertex_buffer_array_stride;
    native_limits.maxInterStageShaderVariables = limits.max_inter_stage_shader_components;
    native_limits.maxInterStageShaderVariables = limits.max_inter_stage_shader_variables;
    native_limits.maxColorAttachments = limits.max_color_attachments;
    native_limits.maxColorAttachmentBytesPerSample = limits.max_color_attachment_bytes_per_sample;
    native_limits.maxComputeWorkgroupStorageSize = limits.max_compute_workgroup_storage_size;
    native_limits.maxComputeInvocationsPerWorkgroup = limits.max_compute_invocations_per_workgroup;
    native_limits.maxComputeWorkgroupSizeX = limits.max_compute_workgroup_size_x;
    native_limits.maxComputeWorkgroupSizeY = limits.max_compute_workgroup_size_y;
    native_limits.maxComputeWorkgroupSizeZ = limits.max_compute_workgroup_size_z;
    native_limits.maxComputeWorkgroupsPerDimension = limits.max_compute_workgroups_per_dimension;
    native_limits.maxImmediateSize = limits.max_immediate_size;
    return native_limits;
}

[[nodiscard]] inline Status FillLimits(WGPUAdapter adapter, Limits* limits) {
    if (adapter == nullptr || limits == nullptr) {
        return Status::kError;
    }

    WGPULimits native_limits = WGPU_LIMITS_INIT;
    if (!wgpuAdapterGetLimits(adapter, &native_limits)) {
        return Status::kError;
    }

    FillLimitsFromNative(native_limits, limits);
    return Status::kSuccess;
}

[[nodiscard]] inline Status FillLimits(WGPUDevice device, Limits* limits) {
    if (device == nullptr || limits == nullptr) {
        return Status::kError;
    }

    WGPULimits native_limits = WGPU_LIMITS_INIT;
    if (wgpuDeviceGetLimits(device, &native_limits) != WGPUStatus_Success) {
        return Status::kError;
    }

    FillLimitsFromNative(native_limits, limits);
    return Status::kSuccess;
}

[[nodiscard]] inline Limits QueryLimits(WGPUAdapter adapter) {
    Limits limits{};
    static_cast<void>(FillLimits(adapter, &limits));
    return limits;
}

[[nodiscard]] inline Limits QueryLimits(WGPUDevice device) {
    Limits limits{};
    static_cast<void>(FillLimits(device, &limits));
    return limits;
}

[[nodiscard]] inline SupportedFeatures QueryFeatures(WGPUAdapter adapter) {
    SupportedFeatures features{};
    if (adapter == nullptr) {
        return features;
    }

    WGPUSupportedFeatures native_features = WGPU_SUPPORTED_FEATURES_INIT;
    wgpuAdapterGetFeatures(adapter, &native_features);
    features.values.reserve(native_features.featureCount);
    for (size_t i = 0; i < native_features.featureCount; ++i) {
        features.values.push_back(FromWgpuFeatureName(native_features.features[i]));
    }

    wgpuSupportedFeaturesFreeMembers(native_features);
    return features;
}

[[nodiscard]] inline SupportedFeatures QueryFeatures(WGPUDevice device) {
    SupportedFeatures features{};
    if (device == nullptr) {
        return features;
    }

    WGPUSupportedFeatures native_features = WGPU_SUPPORTED_FEATURES_INIT;
    wgpuDeviceGetFeatures(device, &native_features);
    features.values.reserve(native_features.featureCount);
    for (size_t i = 0; i < native_features.featureCount; ++i) {
        features.values.push_back(FromWgpuFeatureName(native_features.features[i]));
    }

    wgpuSupportedFeaturesFreeMembers(native_features);
    return features;
}

inline void DeviceLostThunk(
    const WGPUDevice*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void* userdata) {
    auto* callback = static_cast<DeviceLostCallback*>(userdata);
    if (callback != nullptr && *callback) {
        (*callback)(FromWgpuDeviceLostReason(reason), StringFromView(message));
    }
}

inline void UncapturedErrorThunk(
    const WGPUDevice*, WGPUErrorType type, WGPUStringView message, void*, void* userdata) {
    auto* callback = static_cast<UncapturedErrorCallback*>(userdata);
    if (callback != nullptr && *callback) {
        (*callback)(FromWgpuErrorType(type), StringFromView(message));
    }
}

[[nodiscard]] inline DeviceDescriptorNativeStorage BuildDeviceDescriptor(const DeviceDesc& desc) {
    DeviceDescriptorNativeStorage storage{};
    storage.required_features.reserve(desc.required_features.size());
    for (const auto feature : desc.required_features) {
        storage.required_features.push_back(ToWgpuFeatureName(feature));
    }

    storage.native_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    storage.native_desc.label = ToWgpuStringView(desc.label);
    storage.native_desc.requiredFeatureCount = storage.required_features.size();
    storage.native_desc.requiredFeatures =
        storage.required_features.empty() ? nullptr : storage.required_features.data();

    if (desc.required_limits.has_value()) {
        storage.required_limits = ToWgpuLimits(*desc.required_limits);
        storage.native_desc.requiredLimits = &storage.required_limits;
    }

    storage.default_queue = WGPU_QUEUE_DESCRIPTOR_INIT;
    storage.default_queue.label = ToWgpuStringView(desc.default_queue.label);
    storage.native_desc.defaultQueue = storage.default_queue;

    if (desc.device_lost_callback) {
        storage.device_lost_callback = std::make_shared<DeviceLostCallback>(desc.device_lost_callback);
        storage.native_desc.deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
        storage.native_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        storage.native_desc.deviceLostCallbackInfo.callback = DeviceLostThunk;
        storage.native_desc.deviceLostCallbackInfo.userdata2 = storage.device_lost_callback.get();
    }

    if (desc.uncaptured_error_callback) {
        storage.uncaptured_error_callback =
            std::make_shared<UncapturedErrorCallback>(desc.uncaptured_error_callback);
        storage.native_desc.uncapturedErrorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
        storage.native_desc.uncapturedErrorCallbackInfo.callback = UncapturedErrorThunk;
        storage.native_desc.uncapturedErrorCallbackInfo.userdata2 =
            storage.uncaptured_error_callback.get();
    }

    return storage;
}

} // namespace woki::api::wgpu::detail
