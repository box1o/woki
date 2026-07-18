#include <woki/gfx/graph/render_graph.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <queue>

namespace woki::gfx {
namespace {

[[nodiscard]] bool IsImported(const GraphResourceDesc& resource) {
    return resource.origin != GraphResourceOrigin::Transient;
}

[[nodiscard]] Result<void> ValidateResource(const GraphResourceDesc& desc) {
    if (desc.label.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Render graph resource requires a label");
    }
    if (desc.origin == GraphResourceOrigin::Imported &&
        std::holds_alternative<std::monostate>(desc.imported)) {
        return Err(
            ErrorCode::ValidationNullValue, "Imported graph resource requires a resource handle");
    }
    if (desc.origin != GraphResourceOrigin::Imported &&
        !std::holds_alternative<std::monostate>(desc.imported)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Non-imported graph resource cannot contain a resource handle");
    }
    if (desc.kind == GraphResourceKind::Texture && desc.origin == GraphResourceOrigin::Transient &&
        (desc.transient.format == rhi::TextureFormat::Undefined ||
            desc.transient.usage == rhi::TextureUsage::None)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Transient graph texture requires a format and usage");
    }
    if (desc.kind == GraphResourceKind::Buffer && desc.origin == GraphResourceOrigin::Transient &&
        (desc.transient_buffer.size == 0 ||
            desc.transient_buffer.usage == rhi::BufferUsage::None)) {
        return Err(
            ErrorCode::ValidationInvalidState, "Transient graph buffer requires a size and usage");
    }
    if (const auto* buffer = std::get_if<BufferHandle>(&desc.imported)) {
        if (desc.kind != GraphResourceKind::Buffer || !*buffer) {
            return Err(ErrorCode::ValidationInvalidState, "Imported graph buffer is invalid");
        }
    }
    if (const auto* texture = std::get_if<TextureHandle>(&desc.imported)) {
        if (desc.kind != GraphResourceKind::Texture || !*texture) {
            return Err(ErrorCode::ValidationInvalidState, "Imported graph texture is invalid");
        }
    }
    return Ok();
}

void AddDependency(
    std::vector<std::vector<u32>>& dependencies, const u32 pass, const u32 dependency) {
    if (pass == dependency) {
        return;
    }
    auto& values = dependencies[pass];
    if (std::ranges::find(values, dependency) == values.end()) {
        values.push_back(dependency);
    }
}

} // namespace

Result<GraphResource> RenderGraph::AddResource(const GraphResourceDesc& desc) {
    if (resources_.size() >= GraphResource::kInvalidIndex) {
        return Err(ErrorCode::ValidationOutOfRange, "Render graph resource limit reached");
    }
    if (auto validation = ValidateResource(desc); !validation) {
        return Err(validation.error());
    }
    const auto handle = GraphResource::FromIndex(static_cast<u32>(resources_.size()));
    resources_.push_back(desc);
    return Ok(handle);
}

Result<GraphResource> RenderGraph::AddTransientTexture(rhi::TransientDesc desc) {
    const std::string label = desc.label;
    return AddResource({
        .label = label,
        .kind = GraphResourceKind::Texture,
        .origin = GraphResourceOrigin::Transient,
        .transient = std::move(desc),
    });
}

Result<GraphResource> RenderGraph::AddTransientBuffer(rhi::TransientBufferDesc desc) {
    const std::string label = desc.label;
    return AddResource({
        .label = label,
        .kind = GraphResourceKind::Buffer,
        .origin = GraphResourceOrigin::Transient,
        .transient_buffer = std::move(desc),
    });
}

Result<GraphResource> RenderGraph::AddPerFrameTexture(std::string label) {
    return AddResource({
        .label = std::move(label),
        .kind = GraphResourceKind::Texture,
        .origin = GraphResourceOrigin::PerFrame,
    });
}

Result<GraphResource> RenderGraph::AddPerFrameBuffer(std::string label) {
    return AddResource({
        .label = std::move(label),
        .kind = GraphResourceKind::Buffer,
        .origin = GraphResourceOrigin::PerFrame,
    });
}

Result<GraphResource> RenderGraph::Import(BufferHandle buffer, std::string label) {
    if (label.empty()) {
        label = "Imported buffer";
    }
    return AddResource({.label = std::move(label),
        .kind = GraphResourceKind::Buffer,
        .origin = GraphResourceOrigin::Imported,
        .imported = buffer});
}

Result<GraphResource> RenderGraph::Import(TextureHandle texture, std::string label) {
    if (label.empty()) {
        label = "Imported texture";
    }
    return AddResource({.label = std::move(label),
        .kind = GraphResourceKind::Texture,
        .origin = GraphResourceOrigin::Imported,
        .imported = texture});
}

Result<GraphPass> RenderGraph::AddPass(const GraphPassDesc& desc) {
    if (passes_.size() >= GraphPass::kInvalidIndex) {
        return Err(ErrorCode::ValidationOutOfRange, "Render graph pass limit reached");
    }
    if (desc.label.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Render graph pass requires a label");
    }
    const auto handle = GraphPass::FromIndex(static_cast<u32>(passes_.size()));
    passes_.push_back(desc);
    return Ok(handle);
}

const GraphResourceDesc* RenderGraph::Resource(const GraphResource resource) const noexcept {
    return resource && resource.Index() < resources_.size() ? &resources_[resource.Index()]
                                                            : nullptr;
}

const GraphPassDesc* RenderGraph::Pass(const GraphPass pass) const noexcept {
    return pass && pass.Index() < passes_.size() ? &passes_[pass.Index()] : nullptr;
}

Result<CompiledRenderGraph> RenderGraph::Compile() const {
    std::vector<std::vector<u32>> dependencies(passes_.size());
    std::vector<std::optional<u32>> last_writers(resources_.size());
    std::vector<std::vector<u32>> readers(resources_.size());

    for (u32 pass_index = 0; pass_index < passes_.size(); ++pass_index) {
        const auto& pass = passes_[pass_index];
        if (pass.kind == GraphPassKind::Compute &&
            (!pass.colors.empty() || pass.depth || !pass.samples.empty())) {
            return Err(ErrorCode::ValidationInvalidState,
                "Compute graph passes cannot declare render attachments or sampled render inputs");
        }
        if (pass.kind != GraphPassKind::Compute && !pass.storage_textures.empty()) {
            return Err(ErrorCode::ValidationInvalidState,
                "Storage texture inputs are only supported by compute graph passes");
        }
        std::vector<u32> used_resources{};
        used_resources.reserve(pass.resources.size());

        const auto validate_attachment = [&](const GraphResource resource,
                                             const bool requires_write) -> Result<void> {
            if (!resource || resource.Index() >= resources_.size() ||
                resources_[resource.Index()].kind != GraphResourceKind::Texture) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph attachment references an invalid texture");
            }
            const auto use =
                std::ranges::find(pass.resources, resource, &GraphResourceUse::resource);
            if (use == pass.resources.end() ||
                (requires_write && use->access == GraphAccess::Read) ||
                (!requires_write && use->access == GraphAccess::Write)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph attachment access does not match its pass declaration");
            }
            return Ok();
        };

        std::vector<u32> color_slots{};
        for (const auto& color : pass.colors) {
            if (std::ranges::find(color_slots, color.slot) != color_slots.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass contains a duplicate color slot");
            }
            color_slots.push_back(color.slot);
            if (auto status = validate_attachment(color.resource, true); !status) {
                return Err(status.error());
            }
        }
        if (pass.depth) {
            if (auto status = validate_attachment(pass.depth->resource, true); !status) {
                return Err(status.error());
            }
        }
        for (const auto& sample : pass.samples) {
            if (auto status = validate_attachment(sample.resource, false); !status) {
                return Err(status.error());
            }
        }
        std::vector<u32> buffer_inputs{};
        for (const auto& buffer : pass.buffers) {
            if (!buffer.resource || buffer.resource.Index() >= resources_.size() ||
                resources_[buffer.resource.Index()].kind != GraphResourceKind::Buffer) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph buffer input references an invalid buffer");
            }
            if (std::ranges::find(buffer_inputs, buffer.resource.Index()) != buffer_inputs.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass contains a duplicate buffer input");
            }
            buffer_inputs.push_back(buffer.resource.Index());
            if (std::ranges::find(pass.resources, buffer.resource, &GraphResourceUse::resource) ==
                pass.resources.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph buffer input is missing its access declaration");
            }
        }
        std::vector<u32> storage_texture_inputs{};
        for (const auto& texture : pass.storage_textures) {
            if (!texture.resource || texture.resource.Index() >= resources_.size() ||
                resources_[texture.resource.Index()].kind != GraphResourceKind::Texture) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph storage texture input references an invalid texture");
            }
            if (std::ranges::find(storage_texture_inputs, texture.resource.Index()) !=
                storage_texture_inputs.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass contains a duplicate storage texture input");
            }
            storage_texture_inputs.push_back(texture.resource.Index());
            const auto& resource = resources_[texture.resource.Index()];
            if (resource.origin == GraphResourceOrigin::Transient &&
                !HasFlag(resource.transient.usage, rhi::TextureUsage::StorageBinding)) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Transient storage texture requires StorageBinding usage");
            }
            if (std::ranges::find(pass.resources, texture.resource,
                    &GraphResourceUse::resource) == pass.resources.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph storage texture is missing its access declaration");
            }
        }

        for (const GraphPass dependency : pass.depends_on) {
            if (!dependency || dependency.Index() >= passes_.size() ||
                dependency.Index() == pass_index) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass contains an invalid dependency");
            }
            AddDependency(dependencies, pass_index, dependency.Index());
        }

        for (const auto& use : pass.resources) {
            if (!use.resource || use.resource.Index() >= resources_.size()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass references an invalid resource");
            }
            const u32 resource_index = use.resource.Index();
            if (std::ranges::find(used_resources, resource_index) != used_resources.end()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Render graph pass declares the same resource more than once");
            }
            used_resources.push_back(resource_index);

            auto& writer = last_writers[resource_index];
            auto& resource_readers = readers[resource_index];
            const bool reads = use.access != GraphAccess::Write;
            const bool writes = use.access != GraphAccess::Read;
            if (reads && !writer && !IsImported(resources_[resource_index])) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Transient graph resource is read before its first write");
            }
            if (writer) {
                AddDependency(dependencies, pass_index, *writer);
            }
            if (writes) {
                for (const u32 reader : resource_readers) {
                    AddDependency(dependencies, pass_index, reader);
                }
                resource_readers.clear();
                writer = pass_index;
            } else {
                resource_readers.push_back(pass_index);
            }
        }
    }

    std::vector<std::vector<u32>> dependents(passes_.size());
    std::vector<u32> indegrees(passes_.size());
    for (u32 pass = 0; pass < dependencies.size(); ++pass) {
        indegrees[pass] = static_cast<u32>(dependencies[pass].size());
        for (const u32 dependency : dependencies[pass]) {
            dependents[dependency].push_back(pass);
        }
    }

    std::priority_queue<u32, std::vector<u32>, std::greater<>> ready{};
    for (u32 pass = 0; pass < indegrees.size(); ++pass) {
        if (indegrees[pass] == 0) {
            ready.push(pass);
        }
    }

    CompiledRenderGraph compiled{};
    compiled.passes.reserve(passes_.size());
    std::vector<u32> execution_positions(passes_.size());
    while (!ready.empty()) {
        const u32 pass = ready.top();
        ready.pop();
        execution_positions[pass] = static_cast<u32>(compiled.passes.size());
        CompiledGraphPass compiled_pass{.pass = GraphPass::FromIndex(pass)};
        compiled_pass.dependencies.reserve(dependencies[pass].size());
        for (const u32 dependency : dependencies[pass]) {
            compiled_pass.dependencies.push_back(GraphPass::FromIndex(dependency));
        }
        compiled.passes.push_back(std::move(compiled_pass));
        for (const u32 dependent : dependents[pass]) {
            if (--indegrees[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }
    if (compiled.passes.size() != passes_.size()) {
        return Err(ErrorCode::ValidationInvalidState, "Render graph contains a dependency cycle");
    }

    for (u32 resource = 0; resource < resources_.size(); ++resource) {
        std::optional<u32> first{};
        u32 last = 0;
        for (u32 pass = 0; pass < passes_.size(); ++pass) {
            const auto& uses = passes_[pass].resources;
            if (std::ranges::find_if(uses, [resource](const auto& use) {
                    return use.resource.Index() == resource;
                }) == uses.end()) {
                continue;
            }
            const u32 position = execution_positions[pass];
            first = first ? std::min(*first, position) : position;
            last = std::max(last, position);
        }
        if (first) {
            compiled.lifetimes.push_back({
                .resource = GraphResource::FromIndex(resource),
                .first_pass = *first,
                .last_pass = last,
            });
        } else if (resources_[resource].origin == GraphResourceOrigin::Transient) {
            return Err(ErrorCode::ValidationInvalidState,
                "Transient graph resources must be used by at least one pass");
        }
    }
    return Ok(std::move(compiled));
}

std::size_t RenderGraph::ResourceCount() const noexcept { return resources_.size(); }
std::size_t RenderGraph::PassCount() const noexcept { return passes_.size(); }
void RenderGraph::Clear() noexcept {
    resources_.clear();
    passes_.clear();
}

} // namespace woki::gfx
