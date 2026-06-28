#pragma once

#include "forward.hpp"
#include "types.hpp"

#include <optional>
#include <span>
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

struct Origin3D final {
    u32 x{0};
    u32 y{0};
    u32 z{0};
};

struct Extent3D final {
    u32 width{0};
    u32 height{0};
    u32 depth_or_array_layers{0};
};

struct Extent2D final {
    u32 width{0};
    u32 height{0};
};

struct TexelCopyBufferLayout final {
    u64 offset{0};
    u32 bytes_per_row{0};
    u32 rows_per_image{0};
};

struct TexelCopyBufferInfo final {
    TexelCopyBufferLayout layout{};
    void* buffer{nullptr};
};

struct Color final {
    f64 r{0.0};
    f64 g{0.0};
    f64 b{0.0};
    f64 a{0.0};
};

struct CommandBufferDesc final {
    void* next_in_chain{nullptr};
    std::string label{"CommandBuffer"};
};

struct ComputePassDesc final {
    void* next_in_chain{nullptr};
    std::string label{"ComputePass"};
    void* timestamp_writes{nullptr};
};

struct RenderPassDesc final {
    void* next_in_chain{nullptr};
    std::string label{"RenderPass"};
    u32 color_attachment_count{0};
    void* color_attachments{nullptr};
    void* depth_stencil_attachment{nullptr};
    void* occlusion_query_set{nullptr};
    void* timestamp_writes{nullptr};
};

struct TexelCopyTextureInfo final {
    void* texture{nullptr};
    u32 mip_level{0};
    Origin3D origin{};
    TextureAspect aspect{TextureAspect::All};
};

struct CopyTextureForBrowserOptions final {
    bool flip_y{false};
    bool needs_color_space_conversion{false};
    AlphaMode src_alpha_mode{AlphaMode::Unpremultiplied};
};

struct ImageCopyExternalTexture final {
    void* external_texture{nullptr};
    Origin3D origin{};
    Extent2D natural_size{};
};

struct CompilationMessage final {
    CompilationMessageType type{CompilationMessageType::Error};
    std::string message{};
    u64 line_num{0};
    u64 line_pos{0};
    u64 offset{0};
    u64 length{0};
};

struct CompilationInfo final {
    std::vector<CompilationMessage> messages{};
};

struct ComputeStateDesc final {
    void* next_in_chain{nullptr};
    ShaderModule* module{nullptr};
    std::string entry_point{"main"};
    u32 constant_count{0};
    void* constants{nullptr};
};

struct TextureDesc final {
    void* next_in_chain{nullptr};
    Extent3D size{};
    u32 mip_level_count{1};
    u32 sample_count{1};
    TextureDimension dimension{TextureDimension::e2D};
    TextureFormat format{TextureFormat::Undefined};
    TextureUsage usage{};
    std::vector<TextureFormat> view_formats{};
    std::string label{"Texture"};
};

struct BindGroupDesc final {
    void* next_in_chain{nullptr};
    BindGroupLayout* layout{nullptr};
    u32 entry_count{0};
    void* entries{nullptr};
    std::string label{"BindGroup"};
};

struct BindGroupLayoutDesc final {
    void* next_in_chain{nullptr};
    u32 entry_count{0};
    void* entries{nullptr};
    std::string label{"BindGroupLayout"};
};

struct BufferDesc final {
    u64 size{0};
    BufferUsage usage{};
    std::string label{"Buffer"};
};

struct CommandEncoderDesc final {
    void* next_in_chain{nullptr};
    std::string label{"CommandEncoder"};
};

struct ComputePipelineDesc final {
    void* next_in_chain{nullptr};
    PipelineLayout* layout{nullptr};
    ComputeStateDesc compute{};
    std::string label{"ComputePipeline"};
};

struct ExternalTextureDesc final {
    void* next_in_chain{nullptr};
    std::string label{"ExternalTexture"};
};

struct PipelineLayoutDesc final {
    void* next_in_chain{nullptr};
    std::span<BindGroupLayout* const> bind_group_layouts{};
    std::string label{"PipelineLayout"};
};

struct QuerySetDesc final {
    void* next_in_chain{nullptr};
    QueryType type{QueryType::Occlusion};
    u32 count{0};
    std::string label{"QuerySet"};
};

struct RenderBundleEncoderDesc final {
    void* next_in_chain{nullptr};
    std::vector<TextureFormat> color_formats{};
    TextureFormat depth_stencil_format{TextureFormat::Undefined};
    u32 sample_count{1};
    bool depth_read_only{false};
    bool stencil_read_only{false};
    std::string label{"RenderBundleEncoder"};
};

struct RenderPipelineDesc final {
    void* next_in_chain{nullptr};
    PipelineLayout* layout{nullptr};
    void* vertex{nullptr};
    void* primitive{nullptr};
    void* depth_stencil{nullptr};
    void* multisample{nullptr};
    void* fragment{nullptr};
    std::string label{"RenderPipeline"};
};

struct ResourceTableDesc final {
    void* next_in_chain{nullptr};
    std::string label{"ResourceTable"};
};

struct SamplerDesc final {
    void* next_in_chain{nullptr};
    AddressMode address_mode_u{AddressMode::ClampToEdge};
    AddressMode address_mode_v{AddressMode::ClampToEdge};
    AddressMode address_mode_w{AddressMode::ClampToEdge};
    FilterMode mag_filter{FilterMode::Nearest};
    FilterMode min_filter{FilterMode::Nearest};
    MipmapFilterMode mipmap_filter{MipmapFilterMode::Nearest};
    f32 lod_min_clamp{0.0f};
    f32 lod_max_clamp{32.0f};
    CompareFunction compare{CompareFunction::Undefined};
    u16 max_anisotropy{1};
    std::string label{"Sampler"};
};

struct ShaderModuleDesc final {
    void* next_in_chain{nullptr};
    std::string code{};
    std::string label{"ShaderModule"};
};

struct SharedBufferMemoryDesc final {
    void* next_in_chain{nullptr};
    std::string label{"SharedBufferMemory"};
};

struct SharedFenceDesc final {
    void* next_in_chain{nullptr};
    std::string label{"SharedFence"};
};

struct SharedTextureMemoryDesc final {
    void* next_in_chain{nullptr};
    std::string label{"SharedTextureMemory"};
};

struct TexelBufferViewDesc final {
    void* next_in_chain{nullptr};
    std::string label{"TexelBufferView"};
};

struct TextureViewDesc final {
    void* next_in_chain{nullptr};
    TextureFormat format{TextureFormat::Undefined};
    TextureViewDimension dimension{TextureViewDimension::Undefined};
    u32 base_mip_level{0};
    u32 mip_level_count{kMipLevelCountUndefined};
    u32 base_array_layer{0};
    u32 array_layer_count{kArrayLayerCountUndefined};
    TextureAspect aspect{TextureAspect::All};
    std::string label{"TextureView"};
};

struct RenderBundleDesc final {
    void* next_in_chain{nullptr};
    std::string label{"RenderBundle"};
};

struct BindingResourceDesc final {
    void* next_in_chain{nullptr};
};

struct SharedBufferMemoryBeginAccessDesc final {
    void* next_in_chain{nullptr};
};

struct SharedBufferMemoryEndAccessState final {
    void* next_in_chain{nullptr};
};

struct SharedBufferMemoryProperties final {
    void* next_in_chain{nullptr};
};

struct SharedTextureMemoryBeginAccessDesc final {
    void* next_in_chain{nullptr};
};

struct SharedTextureMemoryEndAccessState final {
    void* next_in_chain{nullptr};
};

struct SharedTextureMemoryProperties final {
    void* next_in_chain{nullptr};
};

struct SharedFenceExportInfo final {
    void* next_in_chain{nullptr};
};

} // namespace woki::rhi
