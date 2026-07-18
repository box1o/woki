#include "include/object_data.wgsl"
#include "include/skinning.wgsl"

struct DepthMaterial {
    base_color: vec4<f32>,
    alpha_cutoff: f32,
};

struct SkinnedDepthVertex {
    @location(0) position: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) joints: vec4<u32>,
    @location(4) weights: vec4<f32>,
};

struct DepthVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@group(1) @binding(0) var<uniform> material: DepthMaterial;
@group(1) @binding(1) var base_color_map: texture_2d<f32>;
@group(1) @binding(2) var material_sampler: sampler;

@vertex
fn vertex_main(input: SkinnedDepthVertex) -> DepthVertexOutput {
    let deformation = skin_matrix(input.joints, input.weights);
    var output: DepthVertexOutput;
    output.position = object.view_projection * object.model * deformation *
        vec4<f32>(input.position, 1.0);
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
