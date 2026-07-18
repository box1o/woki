#include <woki/gfx/shader/shader_manager.hpp>
#include <woki/gfx/shader/shader_preprocessor.hpp>

#include "../resource/resource_registry.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>

namespace woki::gfx {
namespace {

struct ShaderStageRecord final {
    ShaderStage stage{ShaderStage::Vertex};
    scope<rhi::ShaderModule> module{};
};

struct ShaderRecord final {
    ShaderDesc desc{};
    std::vector<ShaderStageRecord> stages{};
};

struct CompiledShader final {
    ShaderRecord record{};
    std::vector<paths::Path> source_paths{};
    std::vector<std::string> dependencies{};
};

[[nodiscard]] std::string StageLabel(const ShaderDesc& desc, const ShaderStage stage) {
    std::string label = desc.label.empty() ? "Shader" : desc.label;
    label.push_back('.');
    label.append(ToString(stage));
    return label;
}

} // namespace

Result<void> Validate(const ShaderDesc& desc) {
    if (desc.sources.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Shader requires at least one stage source");
    }

    std::unordered_set<ShaderStage> stages{};
    for (const auto& source : desc.sources) {
        if (!stages.insert(source.stage).second) {
            return Err(ErrorCode::ValidationInvalidState, "Shader contains a duplicate stage");
        }
        if (!source.HasInlineSource() && !source.HasFileSource()) {
            return Err(
                ErrorCode::ValidationNullValue, "Shader stage requires inline or file source");
        }
        if (source.entry_point.empty()) {
            return Err(ErrorCode::ValidationNullValue, "Shader entry point cannot be empty");
        }
        if (source.language != ShaderLanguage::Wgsl) {
            return Err(ErrorCode::GraphicsUnsupportedApi,
                "Only WGSL shader source is currently supported");
        }
    }
    if (stages.contains(ShaderStage::Compute) && stages.size() != 1) {
        return Err(ErrorCode::ValidationInvalidState,
            "Compute stages cannot be mixed with graphics stages");
    }
    return Validate(desc.interface);
}

class ShaderManager::Impl final {
public:
    Impl(rhi::Device& device, std::vector<paths::Path> include_search_paths)
        : device_(&device), includes(std::move(include_search_paths)), preprocessor(includes) {}

    [[nodiscard]] Result<CompiledShader> Compile(const ShaderDesc& desc) {
        CompiledShader compiled{};
        compiled.record.desc = desc;
        compiled.record.stages.reserve(desc.sources.size());

        for (const auto& stage_source : desc.sources) {
            std::string source{};
            std::string source_name = stage_source.source_path;
            if (stage_source.HasInlineSource()) {
                source = stage_source.source;
            } else {
                auto loaded = includes.Load(stage_source.source_path, {});
                if (!loaded) {
                    return Err(loaded.error());
                }
                source = std::move(loaded->source);
                source_name = std::move(loaded->canonical_name);
                compiled.source_paths.emplace_back(source_name);
            }

            auto processed = preprocessor.Process(source, source_name);
            if (!processed) {
                return Err(processed.error());
            }
            compiled.dependencies.insert(compiled.dependencies.end(),
                processed->dependencies.begin(), processed->dependencies.end());

            rhi::ShaderModuleDesc module_desc{
                .code = std::move(processed->source),
                .label = StageLabel(desc, stage_source.stage),
            };
            auto module = device_->CreateShaderModule(module_desc);
            if (!module) {
                return Err(module.error());
            }
            compiled.record.stages.push_back({
                .stage = stage_source.stage,
                .module = std::move(*module),
            });
        }

        std::ranges::sort(compiled.dependencies);
        const auto dependency_end = std::ranges::unique(compiled.dependencies).begin();
        compiled.dependencies.erase(dependency_end, compiled.dependencies.end());
        return Ok(std::move(compiled));
    }

    rhi::Device* device_{nullptr};
    FileShaderIncludeProvider includes;
    ShaderPreprocessor preprocessor;
    ShaderHotReload hot_reload{};
    detail::ResourceRegistry<ShaderRecord, ShaderTag> shaders{};

    struct RetiredShader final {
        u64 after_submission{0};
        std::vector<ShaderStageRecord> stages{};
    };
    std::vector<RetiredShader> retired{};
};

ShaderManager::ShaderManager(rhi::Device& device, std::vector<paths::Path> include_search_paths)
    : impl_(std::make_unique<Impl>(device, std::move(include_search_paths))) {}

ShaderManager::~ShaderManager() { Clear(); }

Result<ShaderHandle> ShaderManager::Create(const ShaderDesc& desc) {
    if (auto validation = Validate(desc); !validation) {
        return Err(validation.error());
    }
    if (const ShaderHandle existing = Find(desc.asset_id); existing) {
        return Ok(existing);
    }

    auto compiled = impl_->Compile(desc);
    if (!compiled) {
        return Err(compiled.error());
    }
    const ShaderHandle handle = impl_->shaders.Create(
        desc.asset_id, desc.label, ResourceState::Resident, std::move(compiled->record));

    if (desc.hot_reload && !compiled->source_paths.empty()) {
        auto tracked =
            impl_->hot_reload.TrackFiles(handle, compiled->source_paths, compiled->dependencies);
        if (!tracked) {
            static_cast<void>(impl_->shaders.Remove(handle));
            return Err(tracked.error());
        }
    }
    return Ok(handle);
}

Result<void> ShaderManager::Reload(const ShaderHandle shader, const u64 retire_after_submission) {
    auto* entry = impl_->shaders.TryGet(shader);
    if (entry == nullptr) {
        return Err(ErrorCode::FailedToAcquireResource, "Shader handle is not active");
    }

    auto compiled = impl_->Compile(entry->value.desc);
    if (!compiled) {
        return Err(compiled.error());
    }
    if (entry->value.desc.hot_reload && !compiled->source_paths.empty()) {
        auto tracked =
            impl_->hot_reload.TrackFiles(shader, compiled->source_paths, compiled->dependencies);
        if (!tracked) {
            return Err(tracked.error());
        }
    }

    impl_->retired.push_back({
        .after_submission = retire_after_submission,
        .stages = std::move(entry->value.stages),
    });
    static_cast<void>(impl_->shaders.Replace(shader, std::move(compiled->record)));
    return Ok();
}

ShaderReloadBatch ShaderManager::PollHotReload() { return impl_->hot_reload.Poll(); }

ShaderHandle ShaderManager::Find(const AssetId asset_id) const noexcept {
    return impl_->shaders.Find(asset_id);
}

rhi::ShaderModule* ShaderManager::Resolve(
    const ShaderHandle shader, const ShaderStage stage) noexcept {
    auto* record = impl_->shaders.TryGetValue(shader);
    if (record == nullptr) {
        return nullptr;
    }
    const auto iterator = std::ranges::find(record->stages, stage, &ShaderStageRecord::stage);
    return iterator == record->stages.end() ? nullptr : iterator->module.get();
}

const rhi::ShaderModule* ShaderManager::Resolve(
    const ShaderHandle shader, const ShaderStage stage) const noexcept {
    const auto* record = impl_->shaders.TryGetValue(shader);
    if (record == nullptr) {
        return nullptr;
    }
    const auto iterator = std::ranges::find(record->stages, stage, &ShaderStageRecord::stage);
    return iterator == record->stages.end() ? nullptr : iterator->module.get();
}

const ResourceMetadata* ShaderManager::Metadata(const ShaderHandle shader) const noexcept {
    const auto* entry = impl_->shaders.TryGet(shader);
    return entry == nullptr ? nullptr : &entry->metadata;
}

const ShaderDesc* ShaderManager::Description(const ShaderHandle shader) const noexcept {
    const auto* record = impl_->shaders.TryGetValue(shader);
    return record == nullptr ? nullptr : &record->desc;
}

bool ShaderManager::Destroy(const ShaderHandle shader) {
    static_cast<void>(impl_->hot_reload.Untrack(shader));
    return impl_->shaders.Remove(shader);
}

bool ShaderManager::Retire(const ShaderHandle shader, const u64 after_submission) {
    auto* record = impl_->shaders.TryGetValue(shader);
    if (record == nullptr) {
        return false;
    }
    impl_->retired.push_back({after_submission, std::move(record->stages)});
    static_cast<void>(impl_->hot_reload.Untrack(shader));
    return impl_->shaders.Remove(shader);
}

void ShaderManager::Collect(const u64 completed_submission) {
    std::erase_if(impl_->retired, [completed_submission](const auto& retired) {
        return retired.after_submission <= completed_submission;
    });
}

std::size_t ShaderManager::Size() const noexcept { return impl_->shaders.Size(); }

std::size_t ShaderManager::RetiredCount() const noexcept { return impl_->retired.size(); }

void ShaderManager::Clear() {
    impl_->hot_reload.Clear();
    impl_->shaders.Clear();
    Collect(std::numeric_limits<u64>::max());
}

} // namespace woki::gfx
