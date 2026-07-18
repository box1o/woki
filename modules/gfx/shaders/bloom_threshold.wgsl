#include "fullscreen_triangle.wgsl"

struct ThresholdParameters {
    threshold: f32,
    soft_knee: f32,
    padding: vec2<f32>,
};

@group(0) @binding(0) var source_color: texture_2d<f32>;
@group(0) @binding(1) var linear_sampler: sampler;
@group(0) @binding(2) var<uniform> parameters: ThresholdParameters;

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> FullscreenVertex {
    return fullscreen_vertex(vertex_index);
}

@fragment
fn fragment_main(input: FullscreenVertex) -> @location(0) vec4<f32> {
    let color = max(textureSample(source_color, linear_sampler, input.uv).rgb, vec3<f32>(0.0));
    let brightness = max(color.r, max(color.g, color.b));
    let knee = max(parameters.soft_knee, 0.0001);
    var soft = clamp(brightness - parameters.threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    let contribution = max(brightness - parameters.threshold, soft) /
        max(brightness, 0.0001);
    return vec4<f32>(color * contribution, 1.0);
}
