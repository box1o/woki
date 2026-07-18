#pragma once

#include "resource_handle.hpp"

namespace woki::gfx {

struct BufferTag;
struct MaterialTag;
struct LightTag;
struct MeshTag;
struct PipelineTag;
struct RenderObjectTag;
struct SamplerTag;
struct ShaderTag;
struct TextureTag;

using BufferHandle = ResourceHandle<BufferTag>;
using MaterialHandle = ResourceHandle<MaterialTag>;
using LightHandle = ResourceHandle<LightTag>;
using MeshHandle = ResourceHandle<MeshTag>;
using PipelineHandle = ResourceHandle<PipelineTag>;
using RenderObjectHandle = ResourceHandle<RenderObjectTag>;
using SamplerHandle = ResourceHandle<SamplerTag>;
using ShaderHandle = ResourceHandle<ShaderTag>;
using TextureHandle = ResourceHandle<TextureTag>;

} // namespace woki::gfx
