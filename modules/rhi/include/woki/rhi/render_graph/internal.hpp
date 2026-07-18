#pragma once

#include "context.hpp"
#include "resources.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <woki/core.hpp>
#include <woki/rhi/forward.hpp>

namespace woki::rhi::render_graph::detail {

enum class ResourceKind : u8 {
    Transient,
    Owned,
    PerFrame,
};

enum class ResourceType : u8 {
    Texture = 0,
    Buffer,
};

struct ResourceRecord final {
    ResourceKind kind{ResourceKind::Transient};
    ResourceType type{ResourceType::Texture};
    TransientDesc transient{};
    TransientBufferDesc transient_buffer{};
    Texture* owned_texture{nullptr};
    Buffer* owned_buffer{nullptr};
    u32 first_pass{kInvalidGraphResource};
    u32 last_pass{kInvalidGraphResource};
};

struct FramebufferRecord final {
    std::vector<std::pair<u32, u32>> colors{};
    u32 depth_resource_id{kInvalidGraphResource};
};

struct ColorOutput final {
    u32 slot{0};
    u32 resource_id{kInvalidGraphResource};
    ColorAttachmentConfig config{};
};

struct DepthOutput final {
    u32 resource_id{kInvalidGraphResource};
    DepthAttachmentConfig config{};
};

struct SampleInput final {
    u32 resource_id{kInvalidGraphResource};
    SampleMode mode{SampleMode::ColorTexture};
};

struct BufferInput final {
    u32 resource_id{kInvalidGraphResource};
};

struct StorageTextureInput final {
    u32 resource_id{kInvalidGraphResource};
};

enum class PassKind : u8 {
    Render,
    Compute,
    Copy,
};

struct CopyOperation final {
    u32 src_resource_id{kInvalidGraphResource};
    u32 dst_resource_id{kInvalidGraphResource};
};

struct PassRecord final {
    std::string debug_name{};
    PassKind kind{PassKind::Render};
    std::optional<u32> framebuffer_id{};
    FramebufferTargetConfig framebuffer_config{};
    std::vector<ColorOutput> colors{};
    std::optional<DepthOutput> depth{};
    std::vector<SampleInput> samples{};
    std::vector<BufferInput> buffers{};
    std::vector<StorageTextureInput> storage_textures{};
    std::vector<CopyOperation> copies{};
    std::function<Result<void>(RenderPassContext&)> render_execute{};
    std::function<Result<void>(ComputePassContext&)> compute_execute{};
    std::function<Result<void>(CopyPassContext&)> copy_execute{};
    void* user_data{nullptr};
};

struct TransientPoolKey final {
    TextureFormat format{TextureFormat::Undefined};
    TextureUsage usage{};
    u32 width{0};
    u32 height{0};

    [[nodiscard]] bool operator==(const TransientPoolKey&) const = default;
};

struct PooledTransientTexture final {
    TransientPoolKey key{};
    scope<Texture> texture{};
    scope<TextureView> view{};
    scope<TextureView> depth_sample_view{};
    u32 last_pass{kInvalidGraphResource};
};

struct PooledTransientBuffer final {
    TransientBufferDesc desc{};
    scope<Buffer> buffer{};
    u32 last_pass{kInvalidGraphResource};
};

struct GraphBlueprint final {
    std::vector<ResourceRecord> resources{};
    std::vector<FramebufferRecord> framebuffers{};
    std::vector<PassRecord> passes{};
    std::unordered_map<std::string, u32> pass_name_to_index{};
};

} // namespace woki::rhi::render_graph::detail
