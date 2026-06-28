#pragma once

#include <woki/rhi/descriptors.hpp>

#include "../wgpu_enums.hpp"
#include "native_helpers.hpp"
#include "string.hpp"

#include <string>
#include <vector>

#include <webgpu/webgpu.h>

namespace woki::rhi::wgpu::detail {

using convert::ToWgpu;
using convert::ToWgpuColorWriteMask;

[[nodiscard]] inline WGPUVertexAttribute ToWgpu(const VertexAttributeDesc& desc) noexcept {
    WGPUVertexAttribute native = WGPU_VERTEX_ATTRIBUTE_INIT;
    native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
    native.format = ToWgpu(desc.format);
    native.offset = desc.offset;
    native.shaderLocation = desc.shader_location;
    return native;
}

[[nodiscard]] inline WGPUOptionalBool ToWgpuOptionalBool(const std::optional<bool>& value) noexcept {
    if (!value.has_value()) {
        return WGPUOptionalBool_Undefined;
    }
    return *value ? WGPUOptionalBool_True : WGPUOptionalBool_False;
}

struct VertexBufferLayoutStorage final {
    std::vector<WGPUVertexAttribute> attributes{};
    WGPUVertexBufferLayout native = WGPU_VERTEX_BUFFER_LAYOUT_INIT;

    explicit VertexBufferLayoutStorage(const VertexBufferLayoutDesc& desc) {
        attributes.reserve(desc.attributes.size());
        for (const VertexAttributeDesc& attribute : desc.attributes) {
            attributes.push_back(ToWgpu(attribute));
        }

        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.stepMode = ToWgpu(desc.step_mode);
        native.arrayStride = desc.array_stride;
        native.attributeCount = attributes.size();
        native.attributes = attributes.empty() ? nullptr : attributes.data();
    }
};

struct VertexStateStorage final {
    std::vector<VertexBufferLayoutStorage> buffer_storage{};
    std::vector<WGPUVertexBufferLayout> buffers{};
    std::string entry_point{};
    WGPUVertexState native = WGPU_VERTEX_STATE_INIT;

    explicit VertexStateStorage(const VertexStateDesc& desc) {
        buffer_storage.reserve(desc.buffers.size());
        buffers.reserve(desc.buffers.size());
        for (const VertexBufferLayoutDesc& buffer : desc.buffers) {
            buffer_storage.emplace_back(buffer);
            buffers.push_back(buffer_storage.back().native);
        }

        entry_point = desc.entry_point;
        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.module = NativeShaderModule(desc.module);
        native.entryPoint = ToStringView(entry_point);
        native.constantCount = desc.constant_count;
        native.constants = static_cast<const WGPUConstantEntry*>(desc.constants);
        native.bufferCount = buffers.size();
        native.buffers = buffers.empty() ? nullptr : buffers.data();
    }
};

struct ColorTargetStateStorage final {
    WGPUColorTargetState native = WGPU_COLOR_TARGET_STATE_INIT;

    explicit ColorTargetStateStorage(const ColorTargetStateDesc& desc) {
        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.format = ToWgpu(desc.format);
        native.blend = static_cast<const WGPUBlendState*>(desc.blend);
        native.writeMask = ToWgpuColorWriteMask(desc.write_mask);
    }
};

struct FragmentStateStorage final {
    std::vector<ColorTargetStateStorage> target_storage{};
    std::vector<WGPUColorTargetState> targets{};
    std::string entry_point{};
    WGPUFragmentState native = WGPU_FRAGMENT_STATE_INIT;

    explicit FragmentStateStorage(const FragmentStateDesc& desc) {
        target_storage.reserve(desc.targets.size());
        targets.reserve(desc.targets.size());
        for (const ColorTargetStateDesc& target : desc.targets) {
            target_storage.emplace_back(target);
            targets.push_back(target_storage.back().native);
        }

        entry_point = desc.entry_point;
        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.module = NativeShaderModule(desc.module);
        native.entryPoint = ToStringView(entry_point);
        native.constantCount = desc.constant_count;
        native.constants = static_cast<const WGPUConstantEntry*>(desc.constants);
        native.targetCount = targets.size();
        native.targets = targets.empty() ? nullptr : targets.data();
    }
};

struct PrimitiveStateStorage final {
    WGPUPrimitiveState native = WGPU_PRIMITIVE_STATE_INIT;

    explicit PrimitiveStateStorage(const PrimitiveStateDesc& desc) {
        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.topology = ToWgpu(desc.topology);
        native.stripIndexFormat = ToWgpu(desc.strip_index_format);
        native.frontFace = ToWgpu(desc.front_face);
        native.cullMode = ToWgpu(desc.cull_mode);
        native.unclippedDepth = desc.unclipped_depth ? WGPU_TRUE : WGPU_FALSE;
    }
};

struct DepthStencilStateStorage final {
    WGPUDepthStencilState native = WGPU_DEPTH_STENCIL_STATE_INIT;

    explicit DepthStencilStateStorage(const DepthStencilStateDesc& desc) {
        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.format = ToWgpu(desc.format);
        native.depthWriteEnabled = ToWgpuOptionalBool(desc.depth_write_enabled);
        native.depthCompare = ToWgpu(desc.depth_compare);
    }
};

struct RenderPipelineTypedDescriptorStorage final {
    std::optional<VertexStateStorage> vertex{};
    std::optional<PrimitiveStateStorage> primitive{};
    std::optional<DepthStencilStateStorage> depth_stencil{};
    std::optional<FragmentStateStorage> fragment{};
    WGPUMultisampleState multisample = WGPU_MULTISAMPLE_STATE_INIT;
    WGPURenderPipelineDescriptor native = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;

    explicit RenderPipelineTypedDescriptorStorage(const RenderPipelineDescTyped& desc) {
        if (desc.vertex != nullptr) {
            vertex.emplace(*desc.vertex);
            native.vertex = vertex->native;
        }
        if (desc.primitive != nullptr) {
            primitive.emplace(*desc.primitive);
            native.primitive = primitive->native;
        }
        if (desc.depth_stencil != nullptr) {
            depth_stencil.emplace(*desc.depth_stencil);
            native.depthStencil = &depth_stencil->native;
        }
        if (desc.multisample != nullptr) {
            multisample = *static_cast<const WGPUMultisampleState*>(desc.multisample);
            native.multisample = multisample;
        }
        if (desc.fragment != nullptr) {
            fragment.emplace(*desc.fragment);
            native.fragment = &fragment->native;
        }

        native.nextInChain = static_cast<WGPUChainedStruct*>(desc.next_in_chain);
        native.label = ToStringView(desc.label);
        native.layout = NativePipelineLayout(desc.layout);
    }
};

} // namespace woki::rhi::wgpu::detail
