#pragma once

#include <woki/rhi/objects.hpp>
#include <woki/rhi/types.hpp>

#include "detail/handle.hpp"
#include "detail/string.hpp"

#include <string_view>

#include <webgpu/webgpu.h>

namespace woki::rhi::wgpu {

template <typename Base, typename Handle, void (*ReleaseFn)(Handle), void (*SetLabelFn)(Handle, WGPUStringView)>
class WgpuObjectImpl final : public Base {
public:
    explicit WgpuObjectImpl(Handle handle) noexcept
        : handle_(handle) {}

    void SetLabel(const std::string_view label) override {
        if (handle_) {
            SetLabelFn(handle_.get(), detail::ToStringView(label));
        }
    }

    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override {
        NativeHandles handles{};
        handles.resource = handle_.get();
        return handles;
    }

    [[nodiscard]] Handle GetNativeHandle() const noexcept { return handle_.get(); }

private:
    detail::GpuHandle<Handle, ReleaseFn> handle_;
};

using WgpuBindGroupImpl = WgpuObjectImpl<BindGroup, WGPUBindGroup, wgpuBindGroupRelease, wgpuBindGroupSetLabel>;
using WgpuBindGroupLayoutImpl =
    WgpuObjectImpl<BindGroupLayout, WGPUBindGroupLayout, wgpuBindGroupLayoutRelease, wgpuBindGroupLayoutSetLabel>;
using WgpuBufferImpl = WgpuObjectImpl<Buffer, WGPUBuffer, wgpuBufferRelease, wgpuBufferSetLabel>;
using WgpuCommandBufferImpl =
    WgpuObjectImpl<CommandBuffer, WGPUCommandBuffer, wgpuCommandBufferRelease, wgpuCommandBufferSetLabel>;
using WgpuCommandEncoderImpl =
    WgpuObjectImpl<CommandEncoder, WGPUCommandEncoder, wgpuCommandEncoderRelease, wgpuCommandEncoderSetLabel>;
using WgpuComputePipelineImpl =
    WgpuObjectImpl<ComputePipeline, WGPUComputePipeline, wgpuComputePipelineRelease, wgpuComputePipelineSetLabel>;
using WgpuExternalTextureImpl = WgpuObjectImpl<ExternalTexture,
    WGPUExternalTexture,
    wgpuExternalTextureRelease,
    wgpuExternalTextureSetLabel>;
using WgpuPipelineLayoutImpl =
    WgpuObjectImpl<PipelineLayout, WGPUPipelineLayout, wgpuPipelineLayoutRelease, wgpuPipelineLayoutSetLabel>;
using WgpuQuerySetImpl = WgpuObjectImpl<QuerySet, WGPUQuerySet, wgpuQuerySetRelease, wgpuQuerySetSetLabel>;
using WgpuRenderBundleEncoderImpl = WgpuObjectImpl<RenderBundleEncoder,
    WGPURenderBundleEncoder,
    wgpuRenderBundleEncoderRelease,
    wgpuRenderBundleEncoderSetLabel>;
using WgpuRenderPipelineImpl =
    WgpuObjectImpl<RenderPipeline, WGPURenderPipeline, wgpuRenderPipelineRelease, wgpuRenderPipelineSetLabel>;
using WgpuResourceTableImpl =
    WgpuObjectImpl<ResourceTable, WGPUResourceTable, wgpuResourceTableRelease, wgpuResourceTableSetLabel>;
using WgpuSamplerImpl = WgpuObjectImpl<Sampler, WGPUSampler, wgpuSamplerRelease, wgpuSamplerSetLabel>;
using WgpuShaderModuleImpl =
    WgpuObjectImpl<ShaderModule, WGPUShaderModule, wgpuShaderModuleRelease, wgpuShaderModuleSetLabel>;
using WgpuSharedBufferMemoryImpl = WgpuObjectImpl<SharedBufferMemory,
    WGPUSharedBufferMemory,
    wgpuSharedBufferMemoryRelease,
    wgpuSharedBufferMemorySetLabel>;
using WgpuSharedFenceImpl =
    WgpuObjectImpl<SharedFence, WGPUSharedFence, wgpuSharedFenceRelease, wgpuSharedFenceSetLabel>;
using WgpuSharedTextureMemoryImpl = WgpuObjectImpl<SharedTextureMemory,
    WGPUSharedTextureMemory,
    wgpuSharedTextureMemoryRelease,
    wgpuSharedTextureMemorySetLabel>;
using WgpuTextureImpl = WgpuObjectImpl<Texture, WGPUTexture, wgpuTextureRelease, wgpuTextureSetLabel>;

} // namespace woki::rhi::wgpu
