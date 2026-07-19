#pragma once

#include <woki/rhi/descriptors.hpp>
#include <woki/rhi/forward.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <woki/core.hpp>

namespace woki::rhi {

class BindGroupBuilder final {
public:
    BindGroupBuilder(Device& device, BindGroupLayout& layout, std::string_view label = "RenderGraphBindGroup");

    BindGroupBuilder& BindTexture(u32 binding, TextureView& view);
    BindGroupBuilder& BindSampler(u32 binding, Sampler& sampler);
    BindGroupBuilder& BindBuffer(u32 binding, Buffer& buffer, u64 offset = 0, u64 size = kWholeSize);

    [[nodiscard]] Result<scope<BindGroup>> Build();

private:
    Device* device_{nullptr};
    BindGroupLayout* layout_{nullptr};
    std::string label_{};
    std::vector<BindGroupEntryDesc> entries_{};
};

} // namespace woki::rhi
