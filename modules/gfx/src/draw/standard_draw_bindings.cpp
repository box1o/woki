#include <woki/gfx/draw/standard_draw_bindings.hpp>

#include <woki/gfx/material/material_parameter_layout.hpp>

#include <algorithm>

namespace woki::gfx {
namespace {

[[nodiscard]] std::vector<u32> MaterialGroups(const ShaderInterfaceDesc& interface) {
    std::vector<u32> groups{};
    if (!interface.parameters.empty()) {
        groups.push_back(interface.parameter_group);
    }
    if (interface.uses_lighting &&
        std::ranges::find(groups, interface.lighting_group) == groups.end()) {
        groups.push_back(interface.lighting_group);
    }
    for (const auto& resource : interface.resources) {
        if (std::ranges::find(groups, resource.group) == groups.end()) {
            groups.push_back(resource.group);
        }
    }
    std::ranges::sort(groups);
    return groups;
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

Result<StandardDrawBindings::MaterialBinding> StandardDrawBindings::BuildBinding(
    const ResolvedDraw& draw) {
    const ShaderDesc* shader = shaders_->Description(draw.material.shader);
    if (shader == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Draw binding shader is no longer active");
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

    MaterialBinding result{.pipeline = draw.pipeline, .material = draw.packet.material};
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
        if (shader->interface.uses_lighting && group == shader->interface.lighting_group) {
            if (!lighting_) {
                return Err(ErrorCode::ValidationNullValue,
                    "Lighting shader requires a frame lighting allocation");
            }
            rhi::Buffer* buffer = resources_->Resolve(lighting_->buffer);
            if (buffer == nullptr) {
                return Err(ErrorCode::FailedToAcquireResource,
                    "Frame lighting buffer is no longer active");
            }
            entries.push_back({
                .binding = shader->interface.lighting_binding,
                .buffer = buffer,
                .offset = lighting_->offset,
                .size = lighting_->size,
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
    Clear();
    for (const auto& draw : draws.draws) {
        if (Find(draw.pipeline, draw.packet.material) != nullptr) {
            continue;
        }
        auto binding = BuildBinding(draw);
        if (!binding) {
            Clear();
            return Err(binding.error());
        }
        materials_.push_back(std::move(*binding));
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

void StandardDrawBindings::ClearLighting() noexcept { lighting_.reset(); }

void StandardDrawBindings::Clear() noexcept { materials_.clear(); }

} // namespace woki::gfx
