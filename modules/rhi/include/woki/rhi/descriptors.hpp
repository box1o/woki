#pragma once

#include "forward.hpp"
#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace woki::rhi {

struct InstanceDesc final {
    std::vector<InstanceFeatureName> required_features{};
    std::optional<InstanceLimits> required_limits{};
    bool enable_validation{true};
    std::string label{"Instance"};
};

struct RequestAdapterDesc final {
    Surface* compatible_surface{nullptr};
    FeatureLevel feature_level{FeatureLevel::Undefined};
    BackendType backend_type{BackendType::Undefined};
    PowerPreference power_preference{PowerPreference::HighPerformance};
    bool force_fallback_adapter{false};
};

struct SurfaceDesc final {
    std::string label{"Surface"};
};

// Matches wgpu::SurfaceDescriptor — `next_in_chain` points to a platform WGPUChainedStruct.
struct SurfaceDescriptor final {
    void* next_in_chain{nullptr};
    std::string label{};
};

struct SurfaceCapabilities final {
    TextureUsage usages{TextureUsage::None};
    std::vector<TextureFormat> formats{};
    std::vector<PresentMode> present_modes{};
    std::vector<CompositeAlphaMode> alpha_modes{};
};

struct SurfaceConfiguration final {
    Device* device{nullptr};
    TextureFormat format{TextureFormat::BGRA8Unorm};
    TextureUsage usage{TextureUsage::RenderAttachment};
    u32 width{0};
    u32 height{0};
    std::vector<TextureFormat> view_formats{};
    CompositeAlphaMode alpha_mode{CompositeAlphaMode::Auto};
    PresentMode present_mode{PresentMode::Fifo};
};

struct SurfaceTexture final {
    SurfaceGetCurrentTextureStatus status{SurfaceGetCurrentTextureStatus::Error};
    NativeHandles handles{};
    bool suboptimal{false};
};

struct AdapterInfo final {
    std::string vendor{};
    std::string architecture{};
    std::string device{};
    std::string description{};
    BackendType backend_type{BackendType::Undefined};
    AdapterType adapter_type{AdapterType::Unknown};
    FeatureLevel feature_level{FeatureLevel::Undefined};
    u32 vendor_id{0};
    u32 device_id{0};
    u32 subgroup_min_size{0};
    u32 subgroup_max_size{0};
};

struct SupportedFeatures final {
    std::vector<FeatureName> values{};

    [[nodiscard]] bool Has(FeatureName value) const noexcept {
        return std::ranges::find(values, value) != values.end();
    }
};

struct Limits final {
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

struct DawnFormatCapabilities final {
    void* next_in_chain{nullptr};
};

struct QueueDesc final {
    std::string label{"Queue"};
};

struct DeviceDesc final {
    bool enable_validation{true};
    bool enable_debug_labels{true};
    std::vector<FeatureName> required_features{};
    std::optional<Limits> required_limits{};
    QueueDesc default_queue{};
    DeviceLostCallback device_lost_callback{};
    UncapturedErrorCallback uncaptured_error_callback{};
    std::string label{"Device"};
};

} // namespace woki::rhi
