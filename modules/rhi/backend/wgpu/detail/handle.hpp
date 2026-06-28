#pragma once

#include <utility>

#include <webgpu/webgpu.h>

namespace woki::rhi::wgpu::detail {

template <typename Handle, void (*ReleaseFn)(Handle)>
class GpuHandle {
public:
    GpuHandle() = default;

    explicit GpuHandle(Handle handle) noexcept
        : handle_(handle) {}

    ~GpuHandle() { reset(); }

    GpuHandle(GpuHandle&& other) noexcept
        : handle_(other.release()) {}

    GpuHandle& operator=(GpuHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    GpuHandle(const GpuHandle&) = delete;
    GpuHandle& operator=(const GpuHandle&) = delete;

    [[nodiscard]] Handle get() const noexcept { return handle_; }
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    void reset(Handle handle = nullptr) noexcept {
        if (handle_ != nullptr) {
            ReleaseFn(handle_);
        }
        handle_ = handle;
    }

    [[nodiscard]] Handle release() noexcept { return std::exchange(handle_, nullptr); }

private:
    Handle handle_{nullptr};
};

using InstanceHandle = GpuHandle<WGPUInstance, wgpuInstanceRelease>;
using SurfaceHandle = GpuHandle<WGPUSurface, wgpuSurfaceRelease>;
using AdapterHandle = GpuHandle<WGPUAdapter, wgpuAdapterRelease>;
using TextureHandle = GpuHandle<WGPUTexture, wgpuTextureRelease>;
using TextureViewHandle = GpuHandle<WGPUTextureView, wgpuTextureViewRelease>;
using DeviceHandle = GpuHandle<WGPUDevice, wgpuDeviceRelease>;
using QueueHandle = GpuHandle<WGPUQueue, wgpuQueueRelease>;
using BufferHandle = GpuHandle<WGPUBuffer, wgpuBufferRelease>;
using CommandBufferHandle = GpuHandle<WGPUCommandBuffer, wgpuCommandBufferRelease>;
using CommandEncoderHandle = GpuHandle<WGPUCommandEncoder, wgpuCommandEncoderRelease>;
using ComputePassEncoderHandle = GpuHandle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease>;
using RenderPassEncoderHandle = GpuHandle<WGPURenderPassEncoder, wgpuRenderPassEncoderRelease>;
using TexelBufferViewHandle = GpuHandle<WGPUTexelBufferView, wgpuTexelBufferViewRelease>;
using RenderBundleHandle = GpuHandle<WGPURenderBundle, wgpuRenderBundleRelease>;
using BindGroupHandle = GpuHandle<WGPUBindGroup, wgpuBindGroupRelease>;
using BindGroupLayoutHandle = GpuHandle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease>;
using ComputePipelineHandle = GpuHandle<WGPUComputePipeline, wgpuComputePipelineRelease>;
using ExternalTextureHandle = GpuHandle<WGPUExternalTexture, wgpuExternalTextureRelease>;
using PipelineLayoutHandle = GpuHandle<WGPUPipelineLayout, wgpuPipelineLayoutRelease>;
using QuerySetHandle = GpuHandle<WGPUQuerySet, wgpuQuerySetRelease>;
using RenderBundleEncoderHandle = GpuHandle<WGPURenderBundleEncoder, wgpuRenderBundleEncoderRelease>;
using RenderPipelineHandle = GpuHandle<WGPURenderPipeline, wgpuRenderPipelineRelease>;
using ResourceTableHandle = GpuHandle<WGPUResourceTable, wgpuResourceTableRelease>;
using SamplerHandle = GpuHandle<WGPUSampler, wgpuSamplerRelease>;
using ShaderModuleHandle = GpuHandle<WGPUShaderModule, wgpuShaderModuleRelease>;
using SharedBufferMemoryHandle = GpuHandle<WGPUSharedBufferMemory, wgpuSharedBufferMemoryRelease>;
using SharedFenceHandle = GpuHandle<WGPUSharedFence, wgpuSharedFenceRelease>;
using SharedTextureMemoryHandle = GpuHandle<WGPUSharedTextureMemory, wgpuSharedTextureMemoryRelease>;

inline void retain(WGPUInstance handle) noexcept {
    if (handle != nullptr) {
        wgpuInstanceAddRef(handle);
    }
}

inline void retain(WGPUAdapter handle) noexcept {
    if (handle != nullptr) {
        wgpuAdapterAddRef(handle);
    }
}

} // namespace woki::rhi::wgpu::detail
