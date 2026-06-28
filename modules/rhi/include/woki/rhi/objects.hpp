#pragma once

#include "types.hpp"

#include <string_view>

namespace woki::rhi {

#define WOKI_RHI_OBJECT(Type)                                                                        \
    class Type {                                                                                     \
    public:                                                                                          \
        virtual ~Type() = default;                                                                   \
        virtual void SetLabel(std::string_view label) = 0;                                           \
        [[nodiscard]] virtual NativeHandles GetNativeHandles() const noexcept = 0;                   \
                                                                                                     \
    protected:                                                                                       \
        Type() = default;                                                                            \
    }

WOKI_RHI_OBJECT(BindGroup);
WOKI_RHI_OBJECT(BindGroupLayout);
WOKI_RHI_OBJECT(Buffer);
WOKI_RHI_OBJECT(CommandBuffer);
WOKI_RHI_OBJECT(CommandEncoder);
WOKI_RHI_OBJECT(ComputePipeline);
WOKI_RHI_OBJECT(ExternalTexture);
WOKI_RHI_OBJECT(PipelineLayout);
WOKI_RHI_OBJECT(QuerySet);
WOKI_RHI_OBJECT(RenderBundleEncoder);
WOKI_RHI_OBJECT(RenderPipeline);
WOKI_RHI_OBJECT(ResourceTable);
WOKI_RHI_OBJECT(Sampler);
WOKI_RHI_OBJECT(ShaderModule);
WOKI_RHI_OBJECT(SharedBufferMemory);
WOKI_RHI_OBJECT(SharedFence);
WOKI_RHI_OBJECT(SharedTextureMemory);
WOKI_RHI_OBJECT(Texture);

#undef WOKI_RHI_OBJECT

} // namespace woki::rhi
