#include "fullscreen_triangle.wgsl"

struct CompositeParameters {
    intensity: f32,
    padding: vec3<f32>,
};

@group(0) @binding(0) var source_color: texture_2d<f32>;
@group(0) @binding(1) var bloom_color: texture_2d<f32>;
@group(0) @binding(2) var linear_sampler: sampler;
@group(0) @binding(3) var<uniform> parameters: CompositeParameters;

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> FullscreenVertex {
    return fullscreen_vertex(vertex_index);
}

@fragment
fn fragment_main(input: FullscreenVertex) -> @location(0) vec4<f32> {
    let source = textureSample(source_color, linear_sampler, input.uv);
    let bloom = textureSample(bloom_color, linear_sampler, input.uv).rgb;
    return vec4<f32>(source.rgb + bloom * parameters.intensity, source.a);
}
