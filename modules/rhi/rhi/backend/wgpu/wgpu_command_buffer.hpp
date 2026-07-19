#pragma once

#include <woki/rhi/command_buffer.hpp>

#include "detail/handle.hpp"

namespace woki::rhi::wgpu {

class WgpuCommandBufferImpl final : public CommandBuffer {
public:
    explicit WgpuCommandBufferImpl(WGPUCommandBuffer command_buffer) noexcept;
    ~WgpuCommandBufferImpl() override = default;

    void SetLabel(std::string_view label) override;
    [[nodiscard]] NativeHandles GetNativeHandles() const noexcept override;

    [[nodiscard]] WGPUCommandBuffer GetNativeCommandBuffer() const noexcept;

private:
    detail::CommandBufferHandle command_buffer_;
};

} // namespace woki::rhi::wgpu
