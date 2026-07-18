# GFX Renderer Architecture

This document captures the high-level structure of the GFX rendering module. GFX sits above RHI and owns rendering policy, reusable GPU resources, scene extraction, render-graph scheduling, and extensibility.

## Module boundaries

![GFX module boundaries](module-boundaries.svg)

- RHI provides raw GPU resources and command encoding.
- GFX owns shaders, pipelines, materials, buffers, textures, scenes, features, and frame scheduling.
- Offline tools compile shaders and cook source assets into runtime artifacts.
- Applications, animation, and independent render modules use the public GFX API.

## Resource relationships

![Rendering resource relationships](resource-interlock.svg)

Resources use stable typed handles. Shaders produce layouts and pipelines; materials reference those pipelines and textures; scene instances reference meshes and materials; draw packets resolve everything during render-pass execution.

Shared resources are cached by content or normalized descriptors. Replaced GPU versions remain alive until their final GPU submission completes.

## Shader hot reload

![Shader hot-reload flow](shader-hot-reload.svg)

Shader files and recursive includes are tracked by dependency. At the beginning of a frame, changed
shaders are synchronously recompiled and their dependent pipelines are rebuilt. A failed shader
compile preserves the previous shader modules; diagnostics report reload and pipeline-rebuild
failures. Transactional replacement of an entire multi-shader set is not implemented yet.

## Frame lifecycle

![Renderer frame lifecycle](frame-lifecycle.svg)

Scene extraction creates an immutable frame snapshot. Optional bounding spheres are culled against a
supplied frustum before opaque and transparent queues are built. Features prepare draw state and
uploads synchronously, then the render graph orders passes, manages transient textures, records
commands, and submits the frame.

## Draw resolution

![Draw-state resolution](draw-resolution.svg)

Draw packets contain logical handles rather than raw RHI pointers. Before graph execution, GFX
resolves active pipelines, standardized binding groups, geometry slices, and draw parameters. The
draw encoder avoids redundant pipeline, vertex-buffer, and index-buffer state changes while encoding
contiguous batches.

## External render modules

Independent render modules integrate through the generic render-feature API. They can manage their own shaders and pipelines, allocate dynamic buffers, register render-graph passes, and render to imported or offscreen targets without owning the RHI device or bypassing graph validation.

The future UI renderer remains separate from GFX and uses these same generic facilities.

## Built-in shading

`StandardShaderLibrary` describes file-backed, hot-reloadable unlit, numeric PBR, textured PBR,
shadowed PBR, skinned PBR, and tone-mapping shaders. Shared WGSL files provide object transforms,
BRDF functions, and lighting functions through the normal shader include system. Standard interfaces
reserve independent binding groups for per-object data, material parameters, frame lighting/shadows,
and skin palettes.

The PBR paths implement base color, emissive, metallic, roughness, normal, occlusion, and alpha-mask
inputs with directional, point, and spot lights. Missing material maps bind deterministic 1x1
fallback textures. A shadow feature renders a selected shadow-casting light into graph-owned depth;
the shadowed PBR variant consumes that texture through pass-time frame bindings. Image-based lighting
and cascaded/atlas shadows remain future extensions.

## Animation and skinning

Animation clips contain independent translation, rotation, and scale tracks over a validated
parent-before-child skeleton. Evaluation supports looped and clamped playback and produces local,
global, and inverse-bind-adjusted skin matrices. Render objects may carry an evaluated skin palette;
scene extraction preserves it through immutable snapshots and the draw-binding layer uploads it as
per-draw storage data for skinned shader variants.

Animation file import is intentionally outside GFX. Importers translate FBX, glTF, or other source
formats into these runtime-neutral clip and skeleton structures.

## Built-in render features

- `ForwardRenderFeature` renders opaque and transparent queues to either the final output or an
  offscreen HDR texture.
- `ShadowRenderFeature` filters shadow casters and publishes fixed-resolution depth plus typed light
  metadata through the graph blackboard. Opaque depth variants remain vertex-only. Masked materials
  use a depth pipeline with `depth_fragment` enabled and the built-in `DepthMasked` implementation,
  which samples only base-color alpha and applies the material cutoff so foliage and cutouts cast
  correct silhouettes.
- `PostProcessFeature` consumes an offscreen graph color with a caller-selected fullscreen pipeline.
  Each instance has a unique label and chooses either an intermediate transient output or the final
  per-frame output, allowing ordered chains such as bloom, color grading, and tone mapping. The
  built-in tone-map shader is one ready-to-use pipeline source. Only the last effect in a chain may
  target `PostProcessOutput::Final`.

Feature metadata uses typed blackboard values, while textures remain declared graph resources. This
keeps transient texture lifetimes inside the graph and creates dependent bind groups only when pass
texture views are available.

## Pipeline and diagnostics API

`BuildStandardMaterialPipeline` derives consistent raster, blend, depth, attachment, and resolver-key
state for forward, transparent, and depth-only material variants. `CreateStandardMaterialPipeline`
creates and registers the result idempotently. A pipeline may use a specialized
`implementation_shader` while retaining the material's forward shader in its resolver key; draw
bindings always follow the resolved implementation shader's interface.

Renderer diagnostics expose the last frame result, hot-reload failures, CPU timings for maintenance,
planning, graph compilation, upload, execution, and total frame work, plus live and retired GFX
resource counts. When the RHI device enables `TimestampQuery`, the renderer also captures whole-graph
GPU duration into a three-frame readback ring. Readback starts only after the caller reports the
submission complete, never stalls rendering, and records the newest available duration and its
submission in `RendererDiagnostics`. Unsupported devices continue without GPU timings.
