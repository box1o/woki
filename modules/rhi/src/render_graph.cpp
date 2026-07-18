#include <woki/rhi/render_graph.hpp>

#include <woki/rhi/command_encoder.hpp>
#include <woki/rhi/compute_pass_encoder.hpp>
#include <woki/rhi/device.hpp>
#include <woki/rhi/objects.hpp>
#include <woki/rhi/queue.hpp>
#include <woki/rhi/render_pass_encoder.hpp>

#include <algorithm>
#include <utility>

namespace woki::rhi {
namespace {

using render_graph::detail::ColorOutput;
using render_graph::detail::CopyOperation;
using render_graph::detail::DepthOutput;
using render_graph::detail::FramebufferRecord;
using render_graph::detail::GraphBlueprint;
using render_graph::detail::PassKind;
using render_graph::detail::PassRecord;
using render_graph::detail::PooledTransientTexture;
using render_graph::detail::ResourceKind;
using render_graph::detail::ResourceRecord;
using render_graph::detail::ResourceType;
using render_graph::detail::SampleInput;
using render_graph::detail::TransientPoolKey;

[[nodiscard]] Extent3D ResolveExtent(const ExtentMode& mode, const u32 width, const u32 height) {
    switch (mode.kind) {
    case ExtentModeKind::Swapchain:
        return Extent3D{width, height, 1};
    case ExtentModeKind::Fixed:
        return Extent3D{mode.width, mode.height, 1};
    case ExtentModeKind::Relative:
        return Extent3D{
            std::max(1u, static_cast<u32>(static_cast<f32>(width) * mode.relative_width)),
            std::max(1u, static_cast<u32>(static_cast<f32>(height) * mode.relative_height)),
            1,
        };
    }
    return Extent3D{width, height, 1};
}

[[nodiscard]] bool IsDepthFormat(const TextureFormat format) noexcept {
    switch (format) {
    case TextureFormat::Depth16Unorm:
    case TextureFormat::Depth24Plus:
    case TextureFormat::Depth24PlusStencil8:
    case TextureFormat::Depth32Float:
    case TextureFormat::Depth32FloatStencil8:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] TextureViewDesc MakeTransientViewDesc(const TransientDesc& desc) {
    TextureViewDesc view_desc{};
    view_desc.label = desc.label.empty() ? "RenderGraphView" : desc.label;
    view_desc.format = desc.format;
    view_desc.usage = desc.usage;
    return view_desc;
}

[[nodiscard]] TextureViewDesc MakeDepthSampleViewDesc(const TransientDesc& desc) {
    TextureViewDesc view_desc{};
    view_desc.label = desc.label.empty() ? "RenderGraphDepthSample" : desc.label + ".DepthSample";
    view_desc.format = TextureFormat::Undefined;
    view_desc.usage = desc.usage;
    view_desc.aspect = TextureAspect::DepthOnly;
    return view_desc;
}

[[nodiscard]] Result<scope<Texture>> CreateTransientTexture(
    Device& device, const TransientDesc& desc, const u32 width, const u32 height) {
    TextureDesc native_desc{};
    native_desc.label = desc.label.empty() ? "RenderGraphTransient" : desc.label;
    native_desc.size = ResolveExtent(desc.extent, width, height);
    native_desc.format = desc.format;
    native_desc.usage = desc.usage;
    native_desc.dimension = TextureDimension::e2D;
    native_desc.mip_level_count = 1;
    native_desc.sample_count = desc.sample_count;
    return device.CreateTexture(native_desc);
}

[[nodiscard]] TransientPoolKey MakePoolKey(
    const TransientDesc& desc, const u32 width, const u32 height) {
    const Extent3D size = ResolveExtent(desc.extent, width, height);
    return TransientPoolKey{
        .format = desc.format,
        .usage = desc.usage,
        .width = size.width,
        .height = size.height,
        .sample_count = desc.sample_count,
    };
}

[[nodiscard]] TexelCopyTextureInfo MakeCopyInfo(Texture& texture) {
    return TexelCopyTextureInfo{
        .texture = texture.GetNativeHandles().resource,
        .mip_level = 0,
        .origin = {},
        .aspect = TextureAspect::All,
    };
}

} // namespace

// --- RenderPassContext ---

RenderPassEncoder& RenderPassContext::encoder() {
    WOKI_ASSERT(pass_ != nullptr);
    return *pass_;
}

Device& RenderPassContext::device() noexcept {
    WOKI_ASSERT(device_ != nullptr);
    return *device_;
}

TextureView& RenderPassContext::color(const u32 slot) {
    WOKI_ASSERT(slot < colors_.size() && colors_[slot] != nullptr);
    return *colors_[slot];
}

TextureView& RenderPassContext::depth() {
    WOKI_ASSERT(depth_ != nullptr);
    return *depth_;
}

bool RenderPassContext::has_depth() const noexcept { return depth_ != nullptr; }

TextureView& RenderPassContext::sample(const u32 slot) {
    WOKI_ASSERT(slot < samples_.size() && samples_[slot] != nullptr);
    return *samples_[slot];
}

u32 RenderPassContext::sample_count() const noexcept { return static_cast<u32>(samples_.size()); }

Buffer& RenderPassContext::buffer(const u32 slot) {
    WOKI_ASSERT(slot < buffers_.size() && buffers_[slot] != nullptr);
    return *buffers_[slot];
}

u32 RenderPassContext::buffer_count() const noexcept { return static_cast<u32>(buffers_.size()); }

BindGroup* RenderPassContext::GetOrCreateBindGroup(
    const std::string_view key, std::function<scope<BindGroup>()> factory) {
    const std::string cache_key(key);
    if (auto it = bind_group_cache_.find(cache_key); it != bind_group_cache_.end()) {
        return it->second.get();
    }

    auto created = factory();
    if (!created) {
        return nullptr;
    }

    BindGroup* raw = created.get();
    bind_group_cache_.emplace(cache_key, std::move(created));
    return raw;
}

// --- BindGroupBuilder ---

BindGroupBuilder::BindGroupBuilder(
    Device& device, BindGroupLayout& layout, const std::string_view label)
    : device_(&device), layout_(&layout), label_(label) {}

BindGroupBuilder& BindGroupBuilder::BindTexture(const u32 binding, TextureView& view) {
    entries_.push_back(BindGroupEntryDesc{
        .binding = binding,
        .texture_view = &view,
    });
    return *this;
}

BindGroupBuilder& BindGroupBuilder::BindSampler(const u32 binding, Sampler& sampler) {
    entries_.push_back(BindGroupEntryDesc{
        .binding = binding,
        .sampler = &sampler,
    });
    return *this;
}

BindGroupBuilder& BindGroupBuilder::BindBuffer(
    const u32 binding, Buffer& buffer, const u64 offset, const u64 size) {
    entries_.push_back(BindGroupEntryDesc{
        .binding = binding,
        .buffer = &buffer,
        .offset = offset,
        .size = size,
    });
    return *this;
}

Result<scope<BindGroup>> BindGroupBuilder::Build() {
    if (device_ == nullptr || layout_ == nullptr) {
        return Err(ErrorCode::InvalidState, "BindGroupBuilder is invalid");
    }

    BindGroupDesc desc{};
    desc.label = label_;
    desc.layout = layout_;
    desc.entries = entries_;
    return device_->CreateBindGroup(desc);
}

// --- CopyPassContext ---

CommandEncoder& CopyPassContext::encoder() {
    WOKI_ASSERT(encoder_ != nullptr);
    return *encoder_;
}

Device& CopyPassContext::device() noexcept {
    WOKI_ASSERT(device_ != nullptr);
    return *device_;
}

Texture& CopyPassContext::src(const u32 index) {
    WOKI_ASSERT(index < sources_.size() && sources_[index] != nullptr);
    return *sources_[index];
}

Texture& CopyPassContext::dst(const u32 index) {
    WOKI_ASSERT(index < destinations_.size() && destinations_[index] != nullptr);
    return *destinations_[index];
}

Result<void> CopyPassContext::CopyAll() {
    if (encoder_ == nullptr) {
        return Err(ErrorCode::InvalidState, "CopyPassContext has no encoder");
    }
    if (sources_.size() != destinations_.size()) {
        return Err(
            ErrorCode::ValidationInvalidState, "CopyPassContext source/destination mismatch");
    }

    for (size_t i = 0; i < sources_.size(); ++i) {
        Texture* source = sources_[i];
        Texture* destination = destinations_[i];
        if (source == nullptr || destination == nullptr) {
            return Err(
                ErrorCode::GraphicsResourceCreationFailed, "CopyPassContext missing texture");
        }

        const Extent3D copy_size{
            std::max(1u, width_),
            std::max(1u, height_),
            1,
        };
        if (auto result = encoder_->CopyTextureToTexture(
                MakeCopyInfo(*source), MakeCopyInfo(*destination), copy_size);
            !result) {
            return result;
        }
    }

    return Ok();
}

// --- ComputePassContext ---

ComputePassEncoder& ComputePassContext::encoder() {
    WOKI_ASSERT(pass_ != nullptr);
    return *pass_;
}

Device& ComputePassContext::device() noexcept {
    WOKI_ASSERT(device_ != nullptr);
    return *device_;
}

Buffer& ComputePassContext::buffer(const u32 slot) {
    WOKI_ASSERT(slot < buffers_.size() && buffers_[slot] != nullptr);
    return *buffers_[slot];
}

u32 ComputePassContext::buffer_count() const noexcept { return static_cast<u32>(buffers_.size()); }

TextureView& ComputePassContext::storage_texture(const u32 slot) {
    WOKI_ASSERT(slot < storage_textures_.size() && storage_textures_[slot] != nullptr);
    return *storage_textures_[slot];
}

u32 ComputePassContext::storage_texture_count() const noexcept {
    return static_cast<u32>(storage_textures_.size());
}

// --- PassBuilder ---

PassBuilder::PassBuilder(RenderGraphBuilder& owner, const u32 pass_index)
    : owner_(&owner), pass_index_(pass_index) {}

PassBuilder& PassBuilder::Target(const Framebuffer framebuffer, FramebufferTargetConfig config) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(framebuffer);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    PassRecord& pass = owner_->blueprint_.passes[pass_index_];
    pass.framebuffer_id = framebuffer.id_;
    pass.framebuffer_config = std::move(config);
    return *this;
}

PassBuilder& PassBuilder::Color(
    const u32 slot, const Resource resource, ColorAttachmentConfig config) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    owner_->blueprint_.passes[pass_index_].colors.push_back(ColorOutput{
        .slot = slot,
        .resource_id = resource.id_,
        .config = config,
    });
    return *this;
}

PassBuilder& PassBuilder::Color(
    const u32 slot, const PerFrameSlot resource, ColorAttachmentConfig config) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    owner_->blueprint_.passes[pass_index_].colors.push_back(ColorOutput{
        .slot = slot,
        .resource_id = resource.id_,
        .config = config,
    });
    return *this;
}

PassBuilder& PassBuilder::Resolve(const u32 slot, const Resource resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    auto& colors = owner_->blueprint_.passes[pass_index_].colors;
    const auto output = std::ranges::find(colors, slot, &ColorOutput::slot);
    WOKI_ASSERT(output != colors.end());
    output->resolve_resource_id = resource.id_;
    return *this;
}

PassBuilder& PassBuilder::Resolve(const u32 slot, const PerFrameSlot resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    auto& colors = owner_->blueprint_.passes[pass_index_].colors;
    const auto output = std::ranges::find(colors, slot, &ColorOutput::slot);
    WOKI_ASSERT(output != colors.end());
    output->resolve_resource_id = resource.id_;
    return *this;
}

PassBuilder& PassBuilder::Depth(const Resource resource, DepthAttachmentConfig config) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    owner_->blueprint_.passes[pass_index_].depth = DepthOutput{
        .resource_id = resource.id_,
        .config = config,
    };
    return *this;
}

PassBuilder& PassBuilder::Depth(const PerFrameSlot resource, DepthAttachmentConfig config) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    owner_->blueprint_.passes[pass_index_].depth = DepthOutput{
        .resource_id = resource.id_,
        .config = config,
    };
    return *this;
}

PassBuilder& PassBuilder::Sample(const Resource resource, const SampleMode mode) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    owner_->blueprint_.passes[pass_index_].samples.push_back(SampleInput{
        .resource_id = resource.id_,
        .mode = mode,
    });
    return *this;
}

PassBuilder& PassBuilder::Buffer(const Resource resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    owner_->blueprint_.passes[pass_index_].buffers.push_back({.resource_id = resource.id_});
    return *this;
}

PassBuilder& PassBuilder::Buffer(const PerFrameSlot resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    owner_->blueprint_.passes[pass_index_].buffers.push_back({.resource_id = resource.id_});
    return *this;
}

PassBuilder& PassBuilder::StorageTexture(const Resource resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    owner_->blueprint_.passes[pass_index_].storage_textures.push_back(
        {.resource_id = resource.id_});
    return *this;
}

PassBuilder& PassBuilder::StorageTexture(const PerFrameSlot resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());
    owner_->blueprint_.passes[pass_index_].storage_textures.push_back(
        {.resource_id = resource.id_});
    return *this;
}

PassBuilder& PassBuilder::Copy(const Resource src, const Resource dst) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(src);
    WOKI_ASSERT(dst);
    WOKI_ASSERT(pass_index_ < owner_->blueprint_.passes.size());

    PassRecord& pass = owner_->blueprint_.passes[pass_index_];
    pass.kind = PassKind::Copy;
    pass.copies.push_back(CopyOperation{
        .src_resource_id = src.id_,
        .dst_resource_id = dst.id_,
    });
    return *this;
}

// --- FramebufferBuilder ---

FramebufferBuilder::FramebufferBuilder(RenderGraphBuilder& owner, const u32 framebuffer_index)
    : owner_(&owner), framebuffer_index_(framebuffer_index) {}

FramebufferBuilder& FramebufferBuilder::Color(const u32 slot, const Resource resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(framebuffer_index_ < owner_->blueprint_.framebuffers.size());

    owner_->blueprint_.framebuffers[framebuffer_index_].colors.emplace_back(slot, resource.id_);
    return *this;
}

FramebufferBuilder& FramebufferBuilder::Depth(const Resource resource) {
    WOKI_ASSERT(owner_ != nullptr);
    WOKI_ASSERT(resource);
    WOKI_ASSERT(framebuffer_index_ < owner_->blueprint_.framebuffers.size());

    owner_->blueprint_.framebuffers[framebuffer_index_].depth_resource_id = resource.id_;
    return *this;
}

Framebuffer FramebufferBuilder::Build() {
    Framebuffer framebuffer{};
    framebuffer.id_ = framebuffer_index_;
    return framebuffer;
}

// --- RenderGraphBuilder ---

RenderGraphBuilder::RenderGraphBuilder(Device& device) : device_(&device) {}

PerFrameSlot RenderGraphBuilder::PerFrame() {
    PerFrameSlot slot{};
    slot.id_ = AllocateResource(ResourceRecord{.kind = ResourceKind::PerFrame});
    return slot;
}

PerFrameSlot RenderGraphBuilder::PerFrameBuffer() {
    PerFrameSlot slot{};
    slot.id_ = AllocateResource(
        ResourceRecord{.kind = ResourceKind::PerFrame, .type = ResourceType::Buffer});
    return slot;
}

Resource RenderGraphBuilder::Transient(TransientDesc desc) {
    Resource resource{};
    resource.id_ = AllocateResource(ResourceRecord{
        .kind = ResourceKind::Transient,
        .transient = std::move(desc),
    });
    return resource;
}

Resource RenderGraphBuilder::TransientBuffer(TransientBufferDesc desc) {
    Resource resource{};
    resource.id_ = AllocateResource(ResourceRecord{
        .kind = ResourceKind::Transient,
        .type = ResourceType::Buffer,
        .transient_buffer = std::move(desc),
    });
    return resource;
}

Resource RenderGraphBuilder::Use(Texture& texture) {
    Resource resource{};
    resource.id_ = AllocateResource(ResourceRecord{
        .kind = ResourceKind::Owned,
        .owned_texture = &texture,
    });
    return resource;
}

Resource RenderGraphBuilder::Use(Buffer& buffer) {
    Resource resource{};
    resource.id_ = AllocateResource(ResourceRecord{
        .kind = ResourceKind::Owned,
        .type = ResourceType::Buffer,
        .owned_buffer = &buffer,
    });
    return resource;
}

FramebufferBuilder RenderGraphBuilder::Framebuffer() {
    const u32 id = AllocateFramebuffer();
    return FramebufferBuilder(*this, id);
}

PassBuilder RenderGraphBuilder::AddPass(const std::string_view debug_name) {
    const u32 id = AllocatePass(debug_name);
    return PassBuilder(*this, id);
}

void RenderGraphBuilder::SetPassData(const std::string_view pass_name, void* user_data) {
    const auto it = blueprint_.pass_name_to_index.find(std::string(pass_name));
    if (it == blueprint_.pass_name_to_index.end()) {
        return;
    }
    blueprint_.passes[it->second].user_data = user_data;
}

Result<scope<RenderGraph>> RenderGraphBuilder::Compile(const u32 width, const u32 height) {
    if (device_ == nullptr) {
        return Err(ErrorCode::GraphicsInitFailed, "RenderGraphBuilder has no device");
    }
    if (width == 0 || height == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "RenderGraph compile requires non-zero size");
    }

    const auto use_resource = [this](const u32 resource_id, const u32 pass_index) {
        if (resource_id >= blueprint_.resources.size()) {
            return;
        }
        auto& resource = blueprint_.resources[resource_id];
        resource.first_pass = resource.first_pass == kInvalidGraphResource
                                  ? pass_index
                                  : std::min(resource.first_pass, pass_index);
        resource.last_pass = resource.last_pass == kInvalidGraphResource
                                 ? pass_index
                                 : std::max(resource.last_pass, pass_index);
    };
    for (u32 pass_index = 0; pass_index < blueprint_.passes.size(); ++pass_index) {
        const PassRecord& pass = blueprint_.passes[pass_index];
        for (const auto& color : pass.colors) {
            use_resource(color.resource_id, pass_index);
            if (color.resolve_resource_id != kInvalidGraphResource) {
                use_resource(color.resolve_resource_id, pass_index);
            }
        }
        if (pass.depth) {
            use_resource(pass.depth->resource_id, pass_index);
        }
        for (const auto& sample : pass.samples) {
            use_resource(sample.resource_id, pass_index);
        }
        for (const auto& buffer : pass.buffers) {
            use_resource(buffer.resource_id, pass_index);
        }
        for (const auto& texture : pass.storage_textures) {
            use_resource(texture.resource_id, pass_index);
        }
        for (const auto& copy : pass.copies) {
            use_resource(copy.src_resource_id, pass_index);
            use_resource(copy.dst_resource_id, pass_index);
        }
        if (pass.framebuffer_id && *pass.framebuffer_id < blueprint_.framebuffers.size()) {
            const auto& framebuffer = blueprint_.framebuffers[*pass.framebuffer_id];
            for (const auto& [slot, resource_id] : framebuffer.colors) {
                (void)slot;
                use_resource(resource_id, pass_index);
            }
            if (framebuffer.depth_resource_id != kInvalidGraphResource) {
                use_resource(framebuffer.depth_resource_id, pass_index);
            }
        }
    }

    for (const ResourceRecord& resource : blueprint_.resources) {
        if (resource.kind == ResourceKind::Transient && resource.type == ResourceType::Texture &&
            resource.transient.sample_count == 0) {
            return Err(ErrorCode::ValidationOutOfRange,
                "RenderGraph transient texture sample count must be nonzero");
        }
        if (resource.kind == ResourceKind::Transient && resource.type == ResourceType::Texture &&
            resource.transient.sample_count > 1 &&
            (HasFlag(resource.transient.usage, TextureUsage::StorageBinding) ||
                HasFlag(resource.transient.usage, TextureUsage::CopySrc) ||
                HasFlag(resource.transient.usage, TextureUsage::CopyDst))) {
            return Err(ErrorCode::ValidationInvalidState,
                "RenderGraph multisampled texture cannot use storage or copy usages");
        }
        if (resource.kind == ResourceKind::Transient && resource.type == ResourceType::Buffer &&
            (resource.transient_buffer.size == 0 ||
                resource.transient_buffer.usage == BufferUsage::None)) {
            return Err(ErrorCode::ValidationInvalidState,
                "RenderGraph transient buffer requires size and usage");
        }
        if (resource.kind == ResourceKind::Transient &&
            resource.first_pass == kInvalidGraphResource) {
            return Err(ErrorCode::ValidationInvalidState,
                "RenderGraph transient resources must be used by at least one pass");
        }
    }

    for (const PassRecord& pass : blueprint_.passes) {
        for (const auto& color : pass.colors) {
            if (color.resource_id >= blueprint_.resources.size() ||
                blueprint_.resources[color.resource_id].type != ResourceType::Texture) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph color attachment references an invalid texture");
            }
            if (color.resolve_resource_id == kInvalidGraphResource) {
                continue;
            }
            if (color.resolve_resource_id >= blueprint_.resources.size() ||
                blueprint_.resources[color.resolve_resource_id].type != ResourceType::Texture ||
                color.resolve_resource_id == color.resource_id) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph color resolve references an invalid texture");
            }
            const auto& source = blueprint_.resources[color.resource_id];
            const auto& target = blueprint_.resources[color.resolve_resource_id];
            if (source.kind == ResourceKind::Transient && source.transient.sample_count <= 1) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph color resolve source must be multisampled");
            }
            if (source.kind == ResourceKind::Transient &&
                !HasFlag(source.transient.usage, TextureUsage::RenderAttachment)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph resolve source requires RenderAttachment usage");
            }
            if (target.kind == ResourceKind::Transient && target.transient.sample_count != 1) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph color resolve target must be single-sampled");
            }
            if (target.kind == ResourceKind::Transient &&
                !HasFlag(target.transient.usage, TextureUsage::RenderAttachment)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph resolve target requires RenderAttachment usage");
            }
            if (source.kind == ResourceKind::Transient && target.kind == ResourceKind::Transient &&
                source.transient.format != target.transient.format) {
                return Err(ErrorCode::GraphicsInvalidFormat,
                    "RenderGraph color resolve formats must match");
            }
        }
        for (const auto& input : pass.buffers) {
            if (input.resource_id >= blueprint_.resources.size() ||
                blueprint_.resources[input.resource_id].type != ResourceType::Buffer) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph pass '" + pass.debug_name + "' references an invalid buffer");
            }
        }
        for (const auto& input : pass.storage_textures) {
            if (input.resource_id >= blueprint_.resources.size() ||
                blueprint_.resources[input.resource_id].type != ResourceType::Texture) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph pass '" + pass.debug_name +
                        "' references an invalid storage texture");
            }
            const auto& resource = blueprint_.resources[input.resource_id];
            if (resource.kind == ResourceKind::Transient &&
                !HasFlag(resource.transient.usage, TextureUsage::StorageBinding)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph transient storage texture requires StorageBinding usage");
            }
            if (resource.kind == ResourceKind::Owned && resource.owned_texture != nullptr &&
                !HasFlag(resource.owned_texture->GetUsage(), TextureUsage::StorageBinding)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph owned storage texture requires StorageBinding usage");
            }
        }
        if (pass.kind == PassKind::Compute) {
            if (!pass.compute_execute || !pass.colors.empty() || pass.depth ||
                pass.framebuffer_id || !pass.samples.empty() || !pass.copies.empty()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph compute pass '" + pass.debug_name + "' has an invalid contract");
            }
            continue;
        }
        if (!pass.storage_textures.empty()) {
            return Err(ErrorCode::ValidationInvalidState,
                "RenderGraph storage textures are only valid on compute passes");
        }
        if (pass.kind == PassKind::Copy || !pass.copies.empty()) {
            if (pass.copies.empty()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph copy pass '" + pass.debug_name + "' has no Copy operations");
            }
            if (!pass.copy_execute) {
                return Err(ErrorCode::ValidationInvalidState,
                    "RenderGraph copy pass '" + pass.debug_name + "' has no Execute callback");
            }
            continue;
        }

        if (!pass.render_execute) {
            return Err(ErrorCode::ValidationInvalidState,
                "RenderGraph pass '" + pass.debug_name + "' has no Execute callback");
        }
    }

    return RenderGraph::Create(*device_, std::move(blueprint_), width, height);
}

u32 RenderGraphBuilder::AllocateResource(ResourceRecord record) {
    const u32 id = static_cast<u32>(blueprint_.resources.size());
    blueprint_.resources.push_back(std::move(record));
    return id;
}

u32 RenderGraphBuilder::AllocateFramebuffer() {
    const u32 id = static_cast<u32>(blueprint_.framebuffers.size());
    blueprint_.framebuffers.emplace_back();
    return id;
}

u32 RenderGraphBuilder::AllocatePass(const std::string_view debug_name) {
    const u32 id = static_cast<u32>(blueprint_.passes.size());
    blueprint_.passes.push_back(PassRecord{.debug_name = std::string(debug_name)});
    blueprint_.pass_name_to_index.emplace(blueprint_.passes.back().debug_name, id);
    return id;
}

// --- RenderGraph ---

Result<scope<RenderGraph>> RenderGraph::Create(
    Device& device, GraphBlueprint blueprint, const u32 width, const u32 height) {
    auto graph = scope<RenderGraph>(new RenderGraph(device, std::move(blueprint), width, height));
    if (auto allocated = graph->AllocateRuntimeResources(width, height); !allocated) {
        return Err(allocated.error());
    }
    return Ok(std::move(graph));
}

RenderGraph::RenderGraph(
    Device& device, GraphBlueprint blueprint, const u32 width, const u32 height)
    : device_(&device), blueprint_(std::move(blueprint)), width_(width), height_(height) {
    runtime_resources_.resize(blueprint_.resources.size());
    for (size_t i = 0; i < blueprint_.resources.size(); ++i) {
        runtime_resources_[i].blueprint = blueprint_.resources[i];
    }
}

Result<void> RenderGraph::AllocateRuntimeResources(const u32 width, const u32 height) {
    width_ = width;
    height_ = height;

    std::vector<RuntimeResource*> transients{};
    for (auto& runtime : runtime_resources_) {
        if (runtime.blueprint.kind == ResourceKind::Transient) {
            transients.push_back(&runtime);
        }
    }
    std::ranges::sort(transients, {},
        [](const RuntimeResource* runtime) { return runtime->blueprint.first_pass; });
    for (RuntimeResource* runtime : transients) {
        auto result = runtime->blueprint.type == ResourceType::Buffer
                          ? AcquireTransientBuffer(*runtime)
                          : AcquireTransientResource(*runtime, width, height);
        if (!result) {
            return result;
        }
    }

    for (RuntimeResource& runtime : runtime_resources_) {
        const ResourceRecord& record = runtime.blueprint;

        if (record.kind == ResourceKind::Owned && record.type == ResourceType::Texture &&
            record.owned_texture != nullptr) {
            if (runtime.view == nullptr) {
                runtime.view = record.owned_texture->CreateView({});
            }
        }
    }

    return Ok();
}

void RenderGraph::ReleaseTransientPool() {
    for (PooledTransientTexture& entry : transient_pool_) {
        entry.last_pass = kInvalidGraphResource;
    }
    for (auto& entry : transient_buffer_pool_) {
        entry.last_pass = kInvalidGraphResource;
    }

    for (RuntimeResource& runtime : runtime_resources_) {
        if (runtime.blueprint.kind == ResourceKind::Transient) {
            runtime.pool_index = kInvalidGraphResource;
            runtime.texture.reset();
            runtime.view.reset();
            runtime.buffer.reset();
        }
    }
}

Result<void> RenderGraph::AcquireTransientBuffer(RuntimeResource& runtime) {
    const auto& desc = runtime.blueprint.transient_buffer;
    if (desc.size == 0 || desc.usage == BufferUsage::None) {
        return Err(ErrorCode::ValidationOutOfRange,
            "RenderGraph transient buffer requires size and usage");
    }
    for (u32 index = 0; index < transient_buffer_pool_.size(); ++index) {
        auto& entry = transient_buffer_pool_[index];
        if (entry.desc.size == desc.size && entry.desc.usage == desc.usage &&
            (entry.last_pass == kInvalidGraphResource ||
                entry.last_pass < runtime.blueprint.first_pass)) {
            entry.last_pass = runtime.blueprint.last_pass;
            runtime.pool_index = index;
            runtime.buffer.reset();
            return Ok();
        }
    }
    auto buffer = device_->CreateBuffer({
        .size = desc.size,
        .usage = desc.usage,
        .label = desc.label.empty() ? "RenderGraphTransientBuffer" : desc.label,
    });
    if (!buffer) {
        return Err(buffer.error());
    }
    runtime.pool_index = static_cast<u32>(transient_buffer_pool_.size());
    transient_buffer_pool_.push_back(
        {.desc = desc, .buffer = std::move(*buffer), .last_pass = runtime.blueprint.last_pass});
    return Ok();
}

Result<void> RenderGraph::AcquireTransientResource(
    RuntimeResource& runtime, const u32 width, const u32 height) {
    const ResourceRecord& record = runtime.blueprint;
    const TransientPoolKey key = MakePoolKey(record.transient, width, height);

    for (u32 pool_index = 0; pool_index < transient_pool_.size(); ++pool_index) {
        PooledTransientTexture& entry = transient_pool_[pool_index];
        if (entry.key != key ||
            (entry.last_pass != kInvalidGraphResource && entry.last_pass >= record.first_pass)) {
            continue;
        }

        entry.last_pass = record.last_pass;
        runtime.pool_index = pool_index;
        runtime.texture.reset();
        runtime.view.reset();
        return Ok();
    }

    auto texture = CreateTransientTexture(*device_, record.transient, width, height);
    if (!texture) {
        return Err(texture.error());
    }

    TextureViewDesc view_desc = MakeTransientViewDesc(record.transient);

    PooledTransientTexture pooled{};
    pooled.key = key;
    pooled.texture = std::move(*texture);
    pooled.view = pooled.texture->CreateView(view_desc);
    if (IsDepthFormat(record.transient.format)) {
        pooled.depth_sample_view =
            pooled.texture->CreateView(MakeDepthSampleViewDesc(record.transient));
    }
    pooled.last_pass = record.last_pass;

    runtime.pool_index = static_cast<u32>(transient_pool_.size());
    transient_pool_.push_back(std::move(pooled));
    return Ok();
}

Result<void> RenderGraph::RebuildForResize(const u32 width, const u32 height) {
    if (width == 0 || height == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "RenderGraph resize requires non-zero size");
    }

    ReleaseTransientPool();
    return AllocateRuntimeResources(width, height);
}

std::size_t RenderGraph::TransientTextureAllocationCount() const noexcept {
    return transient_pool_.size();
}

std::size_t RenderGraph::TransientBufferAllocationCount() const noexcept {
    return transient_buffer_pool_.size();
}

Texture* RenderGraph::ResolveTexture(const u32 resource_id) {
    if (resource_id >= runtime_resources_.size()) {
        return nullptr;
    }

    RuntimeResource& runtime = runtime_resources_[resource_id];
    if (runtime.blueprint.type != ResourceType::Texture) {
        return nullptr;
    }
    switch (runtime.blueprint.kind) {
    case ResourceKind::Transient:
        if (runtime.pool_index < transient_pool_.size()) {
            return transient_pool_[runtime.pool_index].texture.get();
        }
        return runtime.texture.get();
    case ResourceKind::Owned:
        return runtime.blueprint.owned_texture;
    case ResourceKind::PerFrame:
        return nullptr;
    }
    return nullptr;
}

TextureView* RenderGraph::ResolveView(const u32 resource_id) {
    if (resource_id >= runtime_resources_.size()) {
        return nullptr;
    }

    RuntimeResource& runtime = runtime_resources_[resource_id];
    if (runtime.blueprint.type != ResourceType::Texture) {
        return nullptr;
    }
    switch (runtime.blueprint.kind) {
    case ResourceKind::Transient:
        if (runtime.pool_index < transient_pool_.size()) {
            return transient_pool_[runtime.pool_index].view.get();
        }
        return runtime.view.get();
    case ResourceKind::PerFrame:
        return runtime.per_frame_view;
    case ResourceKind::Owned:
        return runtime.view.get() != nullptr ? runtime.view.get() : runtime.per_frame_view;
    }
    return nullptr;
}

Buffer* RenderGraph::ResolveBuffer(const u32 resource_id) {
    if (resource_id >= runtime_resources_.size()) {
        return nullptr;
    }
    RuntimeResource& runtime = runtime_resources_[resource_id];
    if (runtime.blueprint.type != ResourceType::Buffer) {
        return nullptr;
    }
    switch (runtime.blueprint.kind) {
    case ResourceKind::Transient:
        return runtime.pool_index < transient_buffer_pool_.size()
                   ? transient_buffer_pool_[runtime.pool_index].buffer.get()
                   : runtime.buffer.get();
    case ResourceKind::Owned:
        return runtime.blueprint.owned_buffer;
    case ResourceKind::PerFrame:
        return runtime.per_frame_buffer;
    }
    return nullptr;
}

TextureView* RenderGraph::ResolveSampleView(const u32 resource_id, const SampleMode mode) {
    if (mode == SampleMode::DepthTexture && resource_id < runtime_resources_.size()) {
        RuntimeResource& runtime = runtime_resources_[resource_id];
        if (runtime.blueprint.kind == ResourceKind::Transient &&
            runtime.pool_index < transient_pool_.size()) {
            TextureView* depth_view = transient_pool_[runtime.pool_index].depth_sample_view.get();
            if (depth_view != nullptr) {
                return depth_view;
            }
        }
    }
    return ResolveView(resource_id);
}

Result<void> RenderGraph::ExecuteRenderPass(const u32 pass_index, CommandEncoder& encoder,
    const u32 width, const u32 height,
    const std::unordered_map<u32, TextureView*>& per_frame_views) {
    const PassRecord& pass = blueprint_.passes[pass_index];

    std::vector<RenderPassColorAttachmentDesc> color_attachments{};
    std::optional<RenderPassDepthStencilAttachmentDesc> depth_attachment{};

    auto resolve = [&](const u32 resource_id) -> TextureView* {
        if (resource_id < runtime_resources_.size()) {
            RuntimeResource& runtime = runtime_resources_[resource_id];
            if (runtime.blueprint.kind == ResourceKind::PerFrame) {
                if (const auto it = per_frame_views.find(resource_id);
                    it != per_frame_views.end()) {
                    return it->second;
                }
                return runtime.per_frame_view;
            }
        }
        return ResolveView(resource_id);
    };

    if (pass.framebuffer_id.has_value() && *pass.framebuffer_id < blueprint_.framebuffers.size()) {
        const FramebufferRecord& framebuffer = blueprint_.framebuffers[*pass.framebuffer_id];

        u32 max_slot = 0;
        for (const auto& [slot, resource_id] : framebuffer.colors) {
            (void)resource_id;
            max_slot = std::max(max_slot, slot);
        }
        if (!framebuffer.colors.empty()) {
            color_attachments.assign(max_slot + 1, RenderPassColorAttachmentDesc{});
        }

        for (const auto& [slot, resource_id] : framebuffer.colors) {
            TextureView* view = resolve(resource_id);
            if (view == nullptr) {
                return Err(ErrorCode::GraphicsResourceCreationFailed,
                    "RenderGraph pass '" + pass.debug_name + "' missing color view for slot " +
                        std::to_string(slot));
            }

            Color clear_value{0.f, 0.f, 0.f, 1.f};
            if (slot < pass.framebuffer_config.clear_color.size()) {
                clear_value = pass.framebuffer_config.clear_color[slot];
            }

            color_attachments[slot] = RenderPassColorAttachmentDesc{
                .view = view,
                .load_op = LoadOp::Clear,
                .store_op = StoreOp::Store,
                .clear_value = clear_value,
            };
        }

        if (framebuffer.depth_resource_id != kInvalidGraphResource) {
            TextureView* depth_view = resolve(framebuffer.depth_resource_id);
            if (depth_view == nullptr) {
                return Err(ErrorCode::GraphicsResourceCreationFailed,
                    "RenderGraph pass '" + pass.debug_name + "' missing depth view");
            }

            depth_attachment = RenderPassDepthStencilAttachmentDesc{
                .view = depth_view,
                .depth_load_op = LoadOp::Clear,
                .depth_store_op = StoreOp::Store,
                .depth_clear_value = pass.framebuffer_config.clear_depth,
            };
        }
    }

    if (!pass.colors.empty()) {
        u32 max_color_slot = 0;
        for (const ColorOutput& color : pass.colors) {
            max_color_slot = std::max(max_color_slot, color.slot);
        }
        if (color_attachments.size() < max_color_slot + 1) {
            color_attachments.resize(max_color_slot + 1, RenderPassColorAttachmentDesc{});
        }
    }

    for (const ColorOutput& color : pass.colors) {
        TextureView* view = resolve(color.resource_id);
        if (view == nullptr) {
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph pass '" + pass.debug_name + "' missing color attachment view");
        }
        TextureView* resolve_target = nullptr;
        if (color.resolve_resource_id != kInvalidGraphResource) {
            resolve_target = resolve(color.resolve_resource_id);
            if (resolve_target == nullptr) {
                return Err(ErrorCode::GraphicsResourceCreationFailed,
                    "RenderGraph pass '" + pass.debug_name + "' missing color resolve view");
            }
        }

        color_attachments[color.slot] = RenderPassColorAttachmentDesc{
            .view = view,
            .resolve_target = resolve_target,
            .load_op = color.config.load,
            .store_op = color.config.store,
            .clear_value = color.config.clear,
        };
    }

    if (pass.depth.has_value()) {
        TextureView* depth_view = resolve(pass.depth->resource_id);
        if (depth_view == nullptr) {
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph pass '" + pass.debug_name + "' missing depth attachment view");
        }

        depth_attachment = RenderPassDepthStencilAttachmentDesc{
            .view = depth_view,
            .depth_load_op = pass.depth->config.load,
            .depth_store_op = pass.depth->config.store,
            .depth_clear_value = pass.depth->config.clear,
            .depth_read_only = !pass.depth->config.write,
        };
    }

    if (color_attachments.empty() && !depth_attachment.has_value()) {
        return Err(ErrorCode::ValidationInvalidState,
            "RenderGraph pass '" + pass.debug_name + "' has no render targets");
    }

    RenderPassDescTyped pass_desc{};
    pass_desc.label = pass.debug_name;
    pass_desc.color_attachments = color_attachments;
    pass_desc.depth_stencil_attachment =
        depth_attachment.has_value() ? &*depth_attachment : nullptr;

    auto pass_encoder = encoder.BeginRenderPass(pass_desc);
    if (!pass_encoder) {
        return Err(pass_encoder.error());
    }

    RenderPassContext context{};
    context.pass_ = pass_encoder->get();
    context.device_ = device_;
    context.user_data_ = pass.user_data;
    context.width_ = width;
    context.height_ = height;

    context.colors_.resize(color_attachments.size());
    for (size_t i = 0; i < color_attachments.size(); ++i) {
        context.colors_[i] = color_attachments[i].view;
    }
    context.depth_ = depth_attachment.has_value() ? depth_attachment->view : nullptr;

    context.samples_.reserve(pass.samples.size());
    for (const SampleInput& sample : pass.samples) {
        TextureView* view = ResolveSampleView(sample.resource_id, sample.mode);
        if (view == nullptr) {
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph pass '" + pass.debug_name + "' missing sample view");
        }
        context.samples_.push_back(view);
    }
    context.buffers_.reserve(pass.buffers.size());
    for (const auto& input : pass.buffers) {
        Buffer* buffer = ResolveBuffer(input.resource_id);
        if (buffer == nullptr) {
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph pass '" + pass.debug_name + "' missing buffer");
        }
        context.buffers_.push_back(buffer);
    }

    auto executed = pass.render_execute(context);
    pass_encoder->get()->End();
    return executed;
}

Result<void> RenderGraph::ExecuteCopyPass(
    const u32 pass_index, CommandEncoder& encoder, const u32 width, const u32 height) {
    const PassRecord& pass = blueprint_.passes[pass_index];

    CopyPassContext context{};
    context.encoder_ = &encoder;
    context.device_ = device_;
    context.user_data_ = pass.user_data;
    context.width_ = width;
    context.height_ = height;

    context.sources_.reserve(pass.copies.size());
    context.destinations_.reserve(pass.copies.size());
    for (const CopyOperation& copy : pass.copies) {
        Texture* source = ResolveTexture(copy.src_resource_id);
        Texture* destination = ResolveTexture(copy.dst_resource_id);
        if (source == nullptr || destination == nullptr) {
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph copy pass '" + pass.debug_name + "' missing texture");
        }
        context.sources_.push_back(source);
        context.destinations_.push_back(destination);
    }

    return pass.copy_execute(context);
}

Result<void> RenderGraph::ExecuteComputePass(const u32 pass_index, CommandEncoder& encoder,
    const std::unordered_map<u32, TextureView*>& per_frame_views) {
    const PassRecord& pass = blueprint_.passes[pass_index];
    auto pass_encoder = encoder.BeginComputePass({.label = pass.debug_name});
    if (!pass_encoder) {
        return Err(pass_encoder.error());
    }

    ComputePassContext context{};
    context.pass_ = pass_encoder->get();
    context.device_ = device_;
    context.user_data_ = pass.user_data;
    context.buffers_.reserve(pass.buffers.size());
    for (const auto& input : pass.buffers) {
        Buffer* buffer = ResolveBuffer(input.resource_id);
        if (buffer == nullptr) {
            pass_encoder->get()->End();
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph compute pass '" + pass.debug_name + "' missing buffer");
        }
        context.buffers_.push_back(buffer);
    }
    context.storage_textures_.reserve(pass.storage_textures.size());
    for (const auto& input : pass.storage_textures) {
        TextureView* view = nullptr;
        if (input.resource_id < runtime_resources_.size() &&
            runtime_resources_[input.resource_id].blueprint.kind == ResourceKind::PerFrame) {
            if (const auto found = per_frame_views.find(input.resource_id);
                found != per_frame_views.end()) {
                view = found->second;
            }
        } else {
            view = ResolveView(input.resource_id);
        }
        if (view == nullptr) {
            pass_encoder->get()->End();
            return Err(ErrorCode::GraphicsResourceCreationFailed,
                "RenderGraph compute pass '" + pass.debug_name + "' missing storage texture");
        }
        context.storage_textures_.push_back(view);
    }

    auto executed = pass.compute_execute(context);
    pass_encoder->get()->End();
    return executed;
}

RenderGraphFrame RenderGraph::BeginFrame(Device& device, const u32 width, const u32 height) {
    if (width != width_ || height != height_) {
        (void)RebuildForResize(width, height);
    }

    RenderGraphFrame frame(*this, device, width, height);
    auto encoder = device.CreateCommandEncoder({.label = "RenderGraphFrame"});
    if (encoder) {
        frame.encoder_ = std::move(*encoder);
    }
    return frame;
}

// --- RenderGraphFrame ---

RenderGraphFrame::RenderGraphFrame(
    RenderGraph& graph, Device& device, const u32 width, const u32 height)
    : graph_(&graph), device_(&device), width_(width), height_(height) {}

RenderGraphFrame::~RenderGraphFrame() = default;

void RenderGraphFrame::Bind(const PerFrameSlot slot, TextureView* view) {
    if (!slot || graph_ == nullptr) {
        return;
    }

    per_frame_views_[slot.id_] = view;
    if (slot.id_ < graph_->runtime_resources_.size()) {
        graph_->runtime_resources_[slot.id_].per_frame_view = view;
    }
}

void RenderGraphFrame::Bind(const PerFrameSlot slot, Buffer* buffer) {
    if (!slot || graph_ == nullptr || slot.id_ >= graph_->runtime_resources_.size()) {
        return;
    }
    auto& runtime = graph_->runtime_resources_[slot.id_];
    if (runtime.blueprint.type == ResourceType::Buffer) {
        runtime.per_frame_buffer = buffer;
    }
}

Result<void> RenderGraphFrame::CaptureTimestamps(
    QuerySet& query_set, Buffer& resolve_buffer, Buffer& readback_buffer) {
    constexpr u64 kTimestampBytes = sizeof(u64) * 2;
    if (query_set.GetType() != QueryType::Timestamp || query_set.GetCount() < 2) {
        return Err(ErrorCode::ValidationInvalidState,
            "Render graph timestamp capture requires two timestamp queries");
    }
    if (resolve_buffer.GetSize() < kTimestampBytes ||
        !HasFlag(resolve_buffer.GetUsage(), BufferUsage::QueryResolve) ||
        !HasFlag(resolve_buffer.GetUsage(), BufferUsage::CopySrc)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Timestamp resolve buffer requires QueryResolve and CopySrc usage");
    }
    if (readback_buffer.GetSize() < kTimestampBytes ||
        !HasFlag(readback_buffer.GetUsage(), BufferUsage::CopyDst) ||
        !HasFlag(readback_buffer.GetUsage(), BufferUsage::MapRead)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Timestamp readback buffer requires CopyDst and MapRead usage");
    }
    timestamp_query_set_ = &query_set;
    timestamp_resolve_buffer_ = &resolve_buffer;
    timestamp_readback_buffer_ = &readback_buffer;
    return Ok();
}

Result<void> RenderGraphFrame::Execute() {
    if (graph_ == nullptr || device_ == nullptr || !encoder_) {
        return Err(ErrorCode::InvalidState, "RenderGraphFrame is invalid");
    }

    if (timestamp_query_set_ != nullptr) {
        if (auto result = encoder_->WriteTimestamp(*timestamp_query_set_, 0); !result) {
            return result;
        }
    }

    for (size_t pass_index = 0; pass_index < graph_->blueprint_.passes.size(); ++pass_index) {
        const PassRecord& pass = graph_->blueprint_.passes[pass_index];
        Result<void> result = Ok();
        if (pass.kind == PassKind::Compute) {
            result = graph_->ExecuteComputePass(
                static_cast<u32>(pass_index), *encoder_, per_frame_views_);
        } else if (pass.kind == PassKind::Copy || !pass.copies.empty()) {
            result =
                graph_->ExecuteCopyPass(static_cast<u32>(pass_index), *encoder_, width_, height_);
        } else {
            result = graph_->ExecuteRenderPass(
                static_cast<u32>(pass_index), *encoder_, width_, height_, per_frame_views_);
        }
        if (!result) {
            return result;
        }
    }

    if (timestamp_query_set_ != nullptr) {
        if (auto result = encoder_->WriteTimestamp(*timestamp_query_set_, 1); !result) {
            return result;
        }
        if (auto result = encoder_->ResolveQuerySet(
                *timestamp_query_set_, 0, 2, *timestamp_resolve_buffer_, 0);
            !result) {
            return result;
        }
        if (auto result = encoder_->CopyBufferToBuffer(
                *timestamp_resolve_buffer_, 0, *timestamp_readback_buffer_, 0, sizeof(u64) * 2);
            !result) {
            return result;
        }
    }

    auto command_buffer = encoder_->Finish({.label = "RenderGraphSubmit"});
    if (!command_buffer) {
        return Err(command_buffer.error());
    }

    CommandBuffer* buffers[] = {command_buffer->get()};
    return device_->GetQueue().Submit(buffers);
}

} // namespace woki::rhi
