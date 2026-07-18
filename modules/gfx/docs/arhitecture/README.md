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

Shader changes compile asynchronously and are validated before becoming active. A complete replacement is committed at a frame boundary. Compilation or validation failures preserve the previous working shader and pipelines.

## Frame lifecycle

![Renderer frame lifecycle](frame-lifecycle.svg)

Scene changes are committed before an immutable frame snapshot is created. Animation, extraction, visibility, draw preparation, and uploads run as jobs. The render graph then orders passes, manages transient resources, records commands, and submits the frame.

## Draw resolution

![Draw-state resolution](draw-resolution.svg)

Draw packets contain logical handles rather than raw RHI pointers. During execution, GFX resolves the active pipeline version, standardized binding groups, geometry slices, and draw parameters. A command-state cache avoids redundant GPU state changes.

## External render modules

Independent render modules integrate through the generic render-feature API. They can manage their own shaders and pipelines, allocate dynamic buffers, register render-graph passes, and render to imported or offscreen targets without owning the RHI device or bypassing graph validation.

The future UI renderer remains separate from GFX and uses these same generic facilities.
