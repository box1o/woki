#pragma once

#include "handle.hpp"

namespace woki::gfx {

struct BufferTag;
struct MaterialTag;
struct MeshTag;
struct PipelineTag;
struct SamplerTag;
struct ShaderTag;
struct TextureTag;

using BufferHandle      = Handle<BufferTag>;
using MaterialHandle    = Handle<MaterialTag>;
using MeshHandle        = Handle<MeshTag>;
using PipelineHandle    = Handle<PipelineTag>;
using SamplerHandle     = Handle<SamplerTag>;
using ShaderHandle      = Handle<ShaderTag>;
using TextureHandle     = Handle<TextureTag>;

} // namespace woki::gfx
