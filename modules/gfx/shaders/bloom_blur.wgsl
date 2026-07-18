#include "fullscreen_triangle.wgsl"

struct BlurParameters {
    direction: vec2<f32>,
    padding: vec2<f32>,
};

@group(0) @binding(0) var source_color: texture_2d<f32>;
@group(0) @binding(1) var linear_sampler: sampler;
@group(0) @binding(2) var<uniform> parameters: BlurParameters;

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> FullscreenVertex {
    return fullscreen_vertex(vertex_index);
}

@fragment
fn fragment_main(input: FullscreenVertex) -> @location(0) vec4<f32> {
    let extent = vec2<f32>(textureDimensions(source_color));
    let step = parameters.direction / max(extent, vec2<f32>(1.0));
    var color = textureSample(source_color, linear_sampler, input.uv).rgb * 0.227027;
    color += textureSample(source_color, linear_sampler, input.uv + step * 1.384615).rgb * 0.316216;
    color += textureSample(source_color, linear_sampler, input.uv - step * 1.384615).rgb * 0.316216;
    color += textureSample(source_color, linear_sampler, input.uv + step * 3.230769).rgb * 0.070270;
    color += textureSample(source_color, linear_sampler, input.uv - step * 3.230769).rgb * 0.070270;
    return vec4<f32>(color, 1.0);
}
