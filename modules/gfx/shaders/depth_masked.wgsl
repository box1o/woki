#include "include/object_data.wgsl"

struct DepthMaterial {
    base_color: vec4<f32>,
    alpha_cutoff: f32,
};

struct DepthVertexInput {
    @location(0) position: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

struct DepthVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@group(1) @binding(0) var<uniform> material: DepthMaterial;
@group(1) @binding(1) var base_color_map: texture_2d<f32>;
@group(1) @binding(2) var material_sampler: sampler;

@vertex
fn vertex_main(input: DepthVertexInput) -> DepthVertexOutput {
    let world_position = object.model * vec4<f32>(input.position, 1.0);
    var output: DepthVertexOutput;
    output.position = object.view_projection * world_position;
    output.uv = input.uv;
    return output;
}

@fragment
fn fragment_main(input: DepthVertexOutput) {
    let alpha = material.base_color.a *
        textureSample(base_color_map, material_sampler, input.uv).a;
    if (alpha < material.alpha_cutoff) {
        discard;
    }
}
