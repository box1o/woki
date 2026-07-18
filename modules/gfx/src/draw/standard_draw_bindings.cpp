#include <woki/gfx/draw/standard_draw_bindings.hpp>

#include <woki/gfx/material/material_parameter_layout.hpp>

#include <algorithm>

namespace woki::gfx {
namespace {

struct alignas(16) GpuObjectData final {
    math::mat4f model{math::mat4f::identity()};
    math::mat4f view_projection{math::mat4f::identity()};
    math::vec4f view_position{};
};

[[nodiscard]] std::vector<u32> MaterialGroups(const ShaderInterfaceDesc& interface) {
    std::vector<u32> groups{};
    if (!interface.parameters.empty()) {
        groups.push_back(interface.parameter_group);
    }
    for (const auto& resource : interface.resources) {
        if (std::ranges::find(groups, resource.group) == groups.end()) {
            groups.push_back(resource.group);
        }
    }
    std::ranges::sort(groups);
    if (interface.uses_object_transform) {
        std::erase(groups, interface.object_group);
    }
    return groups;
}

[[nodiscard]] TextureHandle DefaultTexture(
    const StandardMaterialResources& defaults, const StringId name) noexcept {
    if (name == material_parameters::kNormalTexture) {
        return defaults.normal;
    }
    if (name == material_parameters::kMetallicRoughnessTexture) {
        return defaults.metallic_roughness;
    }
    if (name == material_parameters::kEmissiveTexture) {
        return defaults.black;
    }
    return defaults.white;
}

} // namespace

StandardDrawBindings::StandardDrawBindings(rhi::Device& device, GpuResourceManager& resources,
    ShaderManager& shaders, FrameUniformBuffer& uniforms, StandardDrawBindingsDesc desc)
    : device_(&device), resources_(&resources), shaders_(&shaders), uniforms_(&uniforms),
      desc_(desc) {}

StandardDrawBindings::~StandardDrawBindings() = default;

StandardDrawBindings::MaterialBinding* StandardDrawBindings::Find(
    const rhi::RenderPipeline* pipeline, const MaterialHandle material) noexcept {
    const auto iterator = std::ranges::find_if(materials_, [pipeline, material](const auto& entry) {
        return entry.pipeline == pipeline && entry.material == material;
    });
    return iterator != materials_.end() ? &*iterator : nullptr;
}

StandardDrawBindings::ObjectBinding* StandardDrawBindings::Find(
    std::vector<ObjectBinding>& bindings, const rhi::RenderPipeline* pipeline,
    const RenderObjectHandle object) noexcept {
    const auto iterator = std::ranges::find_if(bindings, [pipeline, object](const auto& entry) {
        return entry.pipeline == pipeline && entry.object == object;
    });
    return iterator != bindings.end() ? &*iterator : nullptr;
}

StandardDrawBindings::FrameBinding* StandardDrawBindings::FindFrame(
    const rhi::RenderPipeline* pipeline) noexcept {
    const auto iterator = std::ranges::find(frames_, pipeline, &FrameBinding::pipeline);
    return iterator != frames_.end() ? &*iterator : nullptr;
}

Result<StandardDrawBindings::MaterialBinding> StandardDrawBindings::BuildBinding(
    const ResolvedDraw& draw, const RenderPassClass pass) {
    const ShaderDesc* shader = shaders_->Description(draw.material.shader);
    if (shader == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Draw binding shader is no longer active");
    }
    MaterialBinding result{.pipeline = draw.pipeline, .material = draw.packet.material};
    if (pass == RenderPassClass::DepthOnly) {
        return Ok(std::move(result));
    }
    auto layout = BuildMaterialParameterLayout(shader->interface.parameters);
    if (!layout) {
        return Err(layout.error());
    }
    auto packed = PackMaterialParameters(*layout, draw.material.parameters);
    if (!packed) {
        return Err(packed.error());
    }

    std::optional<UniformBufferSlice> uniform_slice{};
    if (!packed->bytes.empty()) {
        auto allocation = uniforms_->Write(packed->bytes);
        if (!allocation) {
            return Err(allocation.error());
        }
        uniform_slice = *allocation;
    }

    for (const u32 group : MaterialGroups(shader->interface)) {
        auto native_layout = draw.pipeline->GetBindGroupLayout(group);
        if (!native_layout) {
            return Err(ErrorCode::FailedToAcquireResource,
                "Pipeline material bind group layout is unavailable");
        }
        std::vector<rhi::BindGroupEntryDesc> entries{};
        if (uniform_slice && group == shader->interface.parameter_group) {
            rhi::Buffer* buffer = resources_->Resolve(uniform_slice->buffer);
            if (buffer == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Material uniform buffer is no longer active");
            }
            entries.push_back({
                .binding = shader->interface.parameter_binding,
                .buffer = buffer,
                .offset = uniform_slice->offset,
                .size = uniform_slice->size,
            });
        }
        for (const auto& binding : shader->interface.resources) {
            if (binding.group != group) {
                continue;
            }
            const auto value = draw.material.parameters.Values().find(binding.name);
            if (value == draw.material.parameters.Values().end()) {
                if (binding.required) {
                    return Err(
                        ErrorCode::ValidationNullValue, "Required material binding is missing");
                }
                rhi::BindGroupEntryDesc entry{.binding = binding.binding};
                if (binding.type == ShaderResourceType::Sampler) {
                    entry.sampler = resources_->Resolve(desc_.defaults.sampler);
                } else if (binding.type == ShaderResourceType::Texture2D) {
                    entry.texture_view =
                        resources_->ResolveView(DefaultTexture(desc_.defaults, binding.name));
                } else {
                    return Err(ErrorCode::GraphicsUnsupportedApi,
                        "No standard fallback exists for this optional shader resource type");
                }
                if (entry.sampler == nullptr && entry.texture_view == nullptr) {
                    return Err(ErrorCode::ValidationNullValue,
                        "Optional material binding requires configured default resources");
                }
                entries.push_back(entry);
                continue;
            }
            rhi::BindGroupEntryDesc entry{.binding = binding.binding};
            if (const auto* texture = std::get_if<TextureHandle>(&value->second)) {
                entry.texture_view = resources_->ResolveView(*texture);
                if (entry.texture_view == nullptr) {
                    return Err(ErrorCode::FailedToAcquireResource,
                        "Material texture view is no longer active");
                }
            } else if (const auto* sampler = std::get_if<SamplerHandle>(&value->second)) {
                entry.sampler = resources_->Resolve(*sampler);
                if (entry.sampler == nullptr) {
                    return Err(
                        ErrorCode::FailedToAcquireResource, "Material sampler is no longer active");
                }
            } else {
                return Err(ErrorCode::ValidationInvalidState,
                    "Material binding has an unsupported value type");
            }
            entries.push_back(entry);
        }
        auto bind_group = device_->CreateBindGroup({
            .layout = native_layout.get(),
            .entries = entries,
            .label = draw.material.label + ".Group" + std::to_string(group),
        });
        if (!bind_group) {
            return Err(bind_group.error());
        }
        result.groups.push_back({.group = group, .binding = std::move(*bind_group)});
    }
    return Ok(std::move(result));
}

Result<void> StandardDrawBindings::Prepare(const ResolvedDrawList& draws) {
    if (snapshot_sequence_ != draws.snapshot_sequence) {
        Clear();
        snapshot_sequence_ = draws.snapshot_sequence;
    }
    for (const auto& draw : draws.draws) {
        if (Find(draw.pipeline, draw.packet.material) != nullptr) {
            continue;
        }
        auto binding = BuildBinding(draw, draws.pass);
        if (!binding) {
            Clear();
            return Err(binding.error());
        }
        materials_.push_back(std::move(*binding));
    }
    objects_.reserve(objects_.size() + draws.draws.size());
    skins_.reserve(skins_.size() + draws.draws.size());
    for (const auto& draw : draws.draws) {
        const ShaderDesc* shader = shaders_->Description(draw.material.shader);
        if (shader == nullptr) {
            Clear();
            return Err(
                ErrorCode::FailedToAcquireResource, "Draw binding shader is no longer active");
        }
        if (!shader->interface.uses_object_transform) {
            continue;
        }
        if (Find(objects_, draw.pipeline, draw.packet.object) == nullptr) {
            const GpuObjectData object_data{
                .model = draw.transform,
                .view_projection = view_.view_projection,
                .view_position = {view_.world_position.x, view_.world_position.y,
                    view_.world_position.z, 1.0F},
            };
            auto allocation = uniforms_->Write(std::as_bytes(std::span{&object_data, 1}));
            if (!allocation) {
                Clear();
                return Err(allocation.error());
            }
            rhi::Buffer* buffer = resources_->Resolve(allocation->buffer);
            auto native_layout = draw.pipeline->GetBindGroupLayout(shader->interface.object_group);
            if (buffer == nullptr || !native_layout) {
                Clear();
                return Err(ErrorCode::FailedToAcquireResource,
                    "Object transform binding resources are unavailable");
            }
            const rhi::BindGroupEntryDesc entry{
                .binding = shader->interface.object_binding,
                .buffer = buffer,
                .offset = allocation->offset,
                .size = allocation->size,
            };
            auto bind_group = device_->CreateBindGroup({
                .layout = native_layout.get(),
                .entries = std::span{&entry, 1},
                .label = draw.material.label + ".Object",
            });
            if (!bind_group) {
                Clear();
                return Err(bind_group.error());
            }
            objects_.push_back(ObjectBinding{
                .pipeline = draw.pipeline,
                .object = draw.packet.object,
                .group = shader->interface.object_group,
                .binding = std::move(*bind_group),
            });
        }
        if (!shader->interface.uses_skinning) {
            continue;
        }
        if (Find(skins_, draw.pipeline, draw.packet.object) != nullptr) {
            continue;
        }
        if (draw.skin_matrices.empty()) {
            Clear();
            return Err(
                ErrorCode::ValidationNullValue, "Skinned shader requires a nonempty joint palette");
        }
        auto skin_allocation = uniforms_->Write(std::as_bytes(std::span{draw.skin_matrices}));
        if (!skin_allocation) {
            Clear();
            return Err(skin_allocation.error());
        }
        rhi::Buffer* skin_buffer = resources_->Resolve(skin_allocation->buffer);
        auto skin_layout = draw.pipeline->GetBindGroupLayout(shader->interface.skin_group);
        if (skin_buffer == nullptr || !skin_layout) {
            Clear();
            return Err(ErrorCode::FailedToAcquireResource,
                "Skin palette binding resources are unavailable");
        }
        const rhi::BindGroupEntryDesc skin_entry{
            .binding = shader->interface.skin_binding,
            .buffer = skin_buffer,
            .offset = skin_allocation->offset,
            .size = skin_allocation->size,
        };
        auto skin_group = device_->CreateBindGroup({
            .layout = skin_layout.get(),
            .entries = std::span{&skin_entry, 1},
            .label = draw.material.label + ".Skin",
        });
        if (!skin_group) {
            Clear();
            return Err(skin_group.error());
        }
        skins_.push_back(ObjectBinding{
            .pipeline = draw.pipeline,
            .object = draw.packet.object,
            .group = shader->interface.skin_group,
            .binding = std::move(*skin_group),
        });
    }
    return Ok();
}

void StandardDrawBindings::Encode(
    rhi::RenderPassEncoder& pass, const ResolvedDraw& draw, const u32) {
    MaterialBinding* binding = Find(draw.pipeline, draw.packet.material);
    WOKI_ASSERT(binding != nullptr);
    for (const auto& group : binding->groups) {
        pass.SetBindGroup(group.group, group.binding.get());
    }
    if (auto* frame = FindFrame(draw.pipeline)) {
        pass.SetBindGroup(frame->group, frame->binding.get());
    }
    if (const auto* object = Find(objects_, draw.pipeline, draw.packet.object)) {
        pass.SetBindGroup(object->group, object->binding.get());
    }
    if (const auto* skin = Find(skins_, draw.pipeline, draw.packet.object)) {
        pass.SetBindGroup(skin->group, skin->binding.get());
    }
    if (desc_.transform_immediate_offset) {
        pass.SetImmediates(
            *desc_.transform_immediate_offset, &draw.transform, sizeof(draw.transform));
    }
}

std::size_t StandardDrawBindings::MaterialBindingCount() const noexcept {
    return materials_.size();
}

Result<void> StandardDrawBindings::SetLighting(const std::span<const std::byte> data) {
    auto allocation = uniforms_->Write(data);
    if (!allocation) {
        return Err(allocation.error());
    }
    lighting_ = *allocation;
    return Ok();
}

Result<void> StandardDrawBindings::SetShadow(const ShadowFrameData& data) {
    auto allocation = uniforms_->Write(std::as_bytes(std::span{&data, 1}));
    if (!allocation) {
        return Err(allocation.error());
    }
    shadow_ = *allocation;
    return Ok();
}

Result<void> StandardDrawBindings::PrepareFrame(rhi::RenderPassContext& context,
    const ResolvedDrawList& draws, const std::optional<u32> shadow_sample) {
    for (const auto& draw : draws.draws) {
        if (FindFrame(draw.pipeline) != nullptr) {
            continue;
        }
        const ShaderDesc* shader = shaders_->Description(draw.material.shader);
        if (shader == nullptr) {
            return Err(
                ErrorCode::FailedToAcquireResource, "Frame binding shader is no longer active");
        }
        if (!shader->interface.uses_lighting) {
            continue;
        }
        if (!lighting_) {
            return Err(ErrorCode::ValidationNullValue,
                "Lighting shader requires a frame lighting allocation");
        }
        rhi::Buffer* lighting_buffer = resources_->Resolve(lighting_->buffer);
        auto layout = draw.pipeline->GetBindGroupLayout(shader->interface.lighting_group);
        if (lighting_buffer == nullptr || !layout) {
            return Err(ErrorCode::FailedToAcquireResource,
                "Frame lighting binding resources are unavailable");
        }
        std::vector<rhi::BindGroupEntryDesc> entries{{
            .binding = shader->interface.lighting_binding,
            .buffer = lighting_buffer,
            .offset = lighting_->offset,
            .size = lighting_->size,
        }};
        if (shader->interface.uses_shadows) {
            if (!shadow_ || !shadow_sample || *shadow_sample >= context.sample_count()) {
                return Err(ErrorCode::ValidationNullValue,
                    "Shadow shader requires frame shadow data and a sampled depth input");
            }
            rhi::Buffer* shadow_buffer = resources_->Resolve(shadow_->buffer);
            rhi::Sampler* sampler = resources_->Resolve(desc_.defaults.shadow_sampler);
            if (shadow_buffer == nullptr || sampler == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Frame shadow binding resources are unavailable");
            }
            entries.push_back({.binding = shader->interface.shadow_data_binding,
                .buffer = shadow_buffer,
                .offset = shadow_->offset,
                .size = shadow_->size});
            entries.push_back({.binding = shader->interface.shadow_texture_binding,
                .texture_view = &context.sample(*shadow_sample)});
            entries.push_back(
                {.binding = shader->interface.shadow_sampler_binding, .sampler = sampler});
        }
        auto group = device_->CreateBindGroup({
            .layout = layout.get(),
            .entries = entries,
            .label = draw.material.label + ".Frame",
        });
        if (!group) {
            return Err(group.error());
        }
        frames_.push_back({.pipeline = draw.pipeline,
            .group = shader->interface.lighting_group,
            .binding = std::move(*group)});
    }
    return Ok();
}

void StandardDrawBindings::SetView(const RenderView& view) noexcept { view_ = view; }

void StandardDrawBindings::ClearLighting() noexcept { lighting_.reset(); }
void StandardDrawBindings::ClearShadow() noexcept { shadow_.reset(); }

void StandardDrawBindings::Clear() noexcept {
    materials_.clear();
    objects_.clear();
    skins_.clear();
    frames_.clear();
    snapshot_sequence_ = 0;
}

} // namespace woki::gfx
